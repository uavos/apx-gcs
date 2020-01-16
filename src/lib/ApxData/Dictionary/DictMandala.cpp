/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "DictMandala.h"

#include <Mandala/flat/Mandala.h>
//=============================================================================
DictMandala::DictMandala(QObject *parent)
    : QObject(parent)
{
    m = new Mandala();

    //special fields (for request & send)
    for (uint var_idx = 0; var_idx < idxPAD; var_idx++) {
        uint type;
        void *value_ptr;
        if (!m->get_ptr(var_idx, &value_ptr, &type))
            break;
        if (type != vt_idx)
            continue;
        const char *var_name;
        const char *var_descr;
        if (!m->get_text_names(var_idx | 0xFF00, &var_name, &var_descr))
            continue;
        special.insert(var_name, var_idx);
    }

    //create values hash
    for (uint var_idx = idxPAD; var_idx < idx_vars_top; var_idx++) {
        uint type;
        void *value_ptr;
        if (!m->get_ptr(var_idx, &value_ptr, &type))
            break;
        const char *var_name;
        const char *var_descr;
        if (!m->get_text_names(var_idx | 0xFF00, &var_name, &var_descr))
            break;
        const char *name;
        const char *descr;
        switch (type) {
        case vt_flag: {
            uint msk = 1;
            while ((msk < 0x0100) && m->get_text_names(var_idx | (msk << 8), &name, &descr)) {
                QString dsc = QString(descr).trimmed();
                QString dsc_tr = qApp->translate("MandalaVars", descr);
                QStringList enumStrings;
                if (dsc.contains('/') && dsc.indexOf('/') > dsc.lastIndexOf(' ')) {
                    // retracted/extracted enum
                    QString sFalse = dsc.right(dsc.size() - dsc.indexOf("/") - 1).trimmed();
                    QString sTrue = dsc.left(dsc.indexOf("/")).trimmed();
                    sTrue.remove(0, dsc.lastIndexOf(' ') + 1);
                    dsc = dsc.left(dsc.lastIndexOf(' '));
                    enumStrings << sFalse.trimmed() << sTrue.trimmed();
                    constants[QString("%1_%2").arg(name).arg(sFalse)] = false;
                    constants[QString("%1_%2").arg(name).arg(sTrue)] = true;
                }
                append(var_idx | (msk << 8),
                       enumStrings.size() ? QMetaType::QStringList : QMetaType::Bool,
                       QString("%1_%2").arg(var_name).arg(name),
                       QString("%1 (%2)").arg(qApp->translate("MandalaVars", descr)).arg(var_name),
                       "bit",
                       enumStrings);
                msk <<= 1;
            }
        } break;
        case vt_enum: {
            QStringList enumStrings;
            uint msk = 0;
            while (m->get_text_names(var_idx | (msk << 8), &name, &descr)) {
                constants[QString("%1_%2").arg(var_name).arg(name)] = msk;
                enumStrings.append(name);
                msk++;
            }
            QString su = "{" + enumStrings.join(", ") + "}";
            append(var_idx,
                   QMetaType::QStringList,
                   var_name,
                   QString("%1 (enum)").arg(qApp->translate("MandalaVars", var_descr)),
                   "enum",
                   enumStrings);
        } break;
        case vt_vect:
        case vt_point: {
            for (int iv = 0; iv < ((type == vt_vect) ? 3 : 2); iv++) {
                QString su, dsc = QString(var_descr).trimmed();
                QString dsc_tr = qApp->translate("MandalaVars", var_descr);
                if (dsc.contains('[')) { // roll,pitch,yaw [deg]
                    su = dsc.right(dsc.size() - dsc.indexOf("[")).trimmed();
                    dsc = dsc.left(dsc.indexOf('[')).trimmed();
                    su.remove('[');
                    su.remove(']');
                    QStringList lsu = su.split(',');
                    if (iv < lsu.size())
                        su = lsu.at(iv);
                    if (dsc_tr.contains('[')) {
                        QString su_tr = dsc_tr.right(dsc_tr.size() - dsc_tr.indexOf("[")).trimmed();
                        dsc_tr = dsc_tr.left(dsc_tr.indexOf('[')).trimmed();
                        su_tr.remove('[');
                        su_tr.remove(']');
                        QStringList lsu = su_tr.split(',');
                        if (iv < lsu.size())
                            su = lsu.at(iv);
                        else
                            su = su_tr;
                    }
                }
                QString vname(var_name);
                if (dsc.contains(":") && dsc.contains(",")) {
                    QString prefix = dsc.left(dsc.indexOf(":")).trimmed();
                    QStringList vlist = dsc.mid(dsc.indexOf(":") + 1).trimmed().split(',');
                    if (!vname.contains("_"))
                        vname.clear();
                    else
                        vname = vname.left(vname.lastIndexOf("_") + 1).trimmed();
                    vname += vlist.at(iv).trimmed();

                    if (dsc_tr.contains(":")) {
                        prefix = dsc_tr.left(dsc_tr.indexOf(":")).trimmed();
                        QStringList st = dsc_tr.mid(dsc_tr.indexOf(":") + 1).trimmed().split(',');
                        if (vlist.size() == st.size())
                            vlist = st;
                    } else
                        prefix = dsc_tr;
                    dsc_tr = prefix + " (" + vlist.at(iv).trimmed() + ")";
                } else {
                    qDebug() << "Mandala descr error:" << var_descr;
                }
                if (!su.isNull()) {
                    dsc_tr += " [" + su + "]";
                }
                append((iv << 8) | var_idx,
                       type == vt_point ? QMetaType::QVector2D : QMetaType::QVector3D,
                       vname,
                       dsc_tr.left(dsc_tr.indexOf('[')).trimmed(),
                       su);
            }
        } break;
        default: {
            QString su, dsc = QString(var_descr).trimmed();
            QString dsc_tr = qApp->translate("MandalaVars", var_descr);
            if (dsc.contains("[")) {
                su = dsc.right(dsc.size() - dsc.indexOf("[")).trimmed();
                su.remove('[');
                su.remove(']');
                if (dsc_tr.contains('['))
                    dsc_tr.truncate(dsc_tr.indexOf('['));
                dsc_tr = QString("%1 [%2]").arg(dsc_tr.trimmed()).arg(su);
            }
            append(var_idx,
                   type == vt_float ? QMetaType::Double : QMetaType::Int,
                   var_name,
                   dsc_tr.left(dsc_tr.indexOf('[')).trimmed(),
                   su);
        }
        } //switch
    }     //for
}
DictMandala::~DictMandala()
{
    delete m;
}
//=============================================================================
void DictMandala::append(quint16 id,
                         QMetaType::Type type,
                         const QString &name,
                         const QString &descr,
                         const QString &units,
                         const QStringList &opts)
{
    Entry i;
    i.id = id;
    i.type = type;
    i.name = name;
    i.descr = descr;
    i.units = units;
    i.opts = opts;

    //value packing support
    i.ptr = nullptr;
    i.vtype = 0;
    m->get_ptr(id, &i.ptr, &i.vtype);

    i.send_set = i.vtype == vt_flag || i.vtype == vt_float || i.vtype == vt_vect
                 || i.vtype == vt_point;

    items.append(i);
    idPos.insert(id, items.size() - 1);

    names.append(name);
}
//=============================================================================
QString DictMandala::hash()
{
    QCryptographicHash h(QCryptographicHash::Sha1);
#define MIDX(...) h.addData(#__VA_ARGS__);
#define MVAR(...) h.addData(#__VA_ARGS__);
#define MBIT(...) h.addData(#__VA_ARGS__);
#include <Mandala/flat/MandalaTemplate.h>

    return h.result().toHex().toUpper();
}
//=============================================================================
double DictMandala::readValue(const Entry &i)
{
    return m->get_data(i.id, i.vtype, i.ptr);
}
QByteArray DictMandala::packValue(const Entry &i, double v)
{
    //qDebug()<<"pack"<<i.id;
    m->set_data(i.id, i.vtype, i.ptr, v);
    int sz = 0;
    sz = m->pack(tmp, i.id);
    return QByteArray((const char *) tmp, sz);
}
QByteArray DictMandala::packSetValue(const Entry &i, double v)
{
    //qDebug()<<"pack"<<i.id;
    m->set_data(i.id, i.vtype, i.ptr, v);
    int sz = 0;
    sz = m->pack_set(tmp, i.id);
    return QByteArray((const char *) tmp, sz);
}
QByteArray DictMandala::packVectorValue(const Entry &i, double v1, double v2, double v3)
{
    const DictMandala::Entry &i1 = items.value(idPos.value(i.id | 0x0000));
    const DictMandala::Entry &i2 = items.value(idPos.value(i.id | 0x0100));
    const DictMandala::Entry &i3 = items.value(idPos.value(i.id | 0x0200));
    m->set_data(i1.id, i1.vtype, i1.ptr, v1);
    m->set_data(i2.id, i2.vtype, i2.ptr, v2);
    m->set_data(i3.id, i3.vtype, i3.ptr, v3);
    int sz = m->pack(tmp, i.id);
    return QByteArray((const char *) tmp, sz);
}
QByteArray DictMandala::packPointValue(const Entry &i, double v1, double v2)
{
    const DictMandala::Entry &i1 = items.value(idPos.value(i.id | 0x0000));
    const DictMandala::Entry &i2 = items.value(idPos.value(i.id | 0x0100));
    m->set_data(i1.id, i1.vtype, i1.ptr, v1);
    m->set_data(i2.id, i2.vtype, i2.ptr, v2);
    int sz = m->pack(tmp, i.id);
    return QByteArray((const char *) tmp, sz);
}
//=============================================================================
bool DictMandala::unpackValue(quint16 id, const QByteArray &data)
{
    //qDebug() << id << data.toHex();
    return m->unpack((quint8 *) data.data(), data.size(), id);
}
bool DictMandala::unpackStream(const QByteArray &data)
{
    return m->unpack_downstream((quint8 *) data.data(), data.size());
}
//=============================================================================
