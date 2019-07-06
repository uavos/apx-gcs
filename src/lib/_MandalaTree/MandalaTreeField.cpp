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
#include "MandalaTreeField.h"
//=============================================================================
MandalaTreeField::MandalaTreeField(MandalaTree *parent,
                                   _mandala_index index,
                                   QString name,
                                   QString descr,
                                   _mandala_field::_mf_type dtype,
                                   QStringList opts,
                                   QStringList optsDescr,
                                   QString alias)
    : MandalaTree(parent, index, name, descr)
    , m_dtype(dtype)
    , m_alias(alias)
    , m_opts(opts)
    , m_optsDescr(optsDescr)
    , m_used(false)
    , m_precision(0)
{
    //meta property
    QVariant vx = qVariantFromValue(this);
    parent->setProperty(m_name.toUtf8().data(), vx);

    //get options from descr
    if (m_dtype == _mandala_field::mf_bit) {
        QString s = descr.mid(descr.lastIndexOf(' ') + 1).trimmed();
        if (s.contains('/')) {
            m_opts << s.mid(s.indexOf('/') + 1);
            m_opts << s.left(s.indexOf('/'));
        } else
            m_opts << tr("no") << tr("yes");
    }
    //get units from descr
    QString su, dsc = m_descr.trimmed();
    QString dsc_tr = qApp->translate("MandalaTreeField", m_descr.toLocal8Bit().data());
    m_descr = dsc_tr;
    if (dsc.contains("[")) {
        su = dsc.right(dsc.size() - dsc.indexOf("[")).trimmed();
        su.remove('[');
        su.remove(']');
        m_units = su.trimmed();
        if (dsc_tr.contains('['))
            dsc_tr.truncate(dsc_tr.indexOf('['));
        dsc_tr = QString("%1 [%2]").arg(dsc_tr.trimmed()).arg(su);
    }
    m_caption = dsc_tr;
    //assume precision
    if (m_name.contains("lat") || m_name.contains("lon"))
        m_precision = 8;
    else if (m_name == "ldratio")
        m_precision = 2;
    else if (m_name.startsWith("ctr"))
        m_precision = 3;
    else if (m_units == "0..1")
        m_precision = 3;
    else if (m_units == "-1..0..+1")
        m_precision = 3;
    else if (m_units == "deg")
        m_precision = 2;
    else if (m_units == "deg/s")
        m_precision = 2;
    else if (m_units == "m")
        m_precision = 2;
    else if (m_units == "m/s")
        m_precision = 2;
    else if (m_units == "m/s2")
        m_precision = 2;
    else if (m_units == "a.u.")
        m_precision = 2;
    else if (m_units == "v")
        m_precision = 2;
    else if (m_units == "A")
        m_precision = 3;
    else if (m_units == "C")
        m_precision = 1;
    else
        m_precision = 6;

    emit structChanged(this);
}
//=============================================================================
//=============================================================================
QVariant MandalaTreeField::value(void) const
{
    if (bindItem)
        return bindItem->value();
    if (m_itype != it_field)
        return QVariant();
    switch (m_dtype) {
    case _mandala_field::mf_float:
        return m_value.toDouble();
    case _mandala_field::mf_uint:
        return m_value.toUInt();
    case _mandala_field::mf_byte:
        return m_value.toUInt();
    case _mandala_field::mf_bit:
        return m_value.toBool();
    case _mandala_field::mf_enum:
        return m_value.toUInt();
    default:
        return QVariant();
    }
}
//=============================================================================
bool MandalaTreeField::setValue(const QVariant &v)
{
    if (bindItem)
        return bindItem->setValue(v);
    if (m_itype != it_field)
        return false;
    QVariant new_value;
    switch (m_dtype) {
    case _mandala_field::mf_float:
        new_value = v.toDouble();
        break;
    case _mandala_field::mf_uint:
        new_value = v.toUInt();
        break;
    case _mandala_field::mf_byte:
        new_value = v.toUInt();
        break;
    case _mandala_field::mf_bit:
    case _mandala_field::mf_enum: {
        QString s = v.toString();
        if (s.contains('\n'))
            s = s.left(s.indexOf('\n'));
        if (m_opts.contains(s))
            new_value = m_opts.indexOf(s);
        else if (v.canConvert(QMetaType::UInt) && (int) v.toUInt() <= m_opts.size())
            new_value = v.toUInt();
        else
            return false;
    } break;
    default:
        return false;
    }
    if (m_value == new_value)
        return true;
    m_value = new_value;
    //qDebug()<<"changed";
    emit valueChanged(this);
    return true;
}
//=============================================================================
const QString &MandalaTreeField::alias(void) const
{
    return m_alias;
}
const QStringList &MandalaTreeField::opts(void) const
{
    return m_opts;
}
const QStringList &MandalaTreeField::optsDescr(void) const
{
    return m_optsDescr;
}
quint8 MandalaTreeField::dtype(void) const
{
    return m_dtype;
}
bool MandalaTreeField::used(void) const
{
    return m_used;
}
void MandalaTreeField::setUsed(bool v)
{
    if (v == m_used)
        return;
    m_used = v;
    emit usedChanged(this);
}
QString MandalaTreeField::valueText(void) const
{
    if (!m_opts.isEmpty())
        return m_opts.value(value().toUInt());
    if (m_dtype == _mandala_field::mf_float)
        return QString("%1").arg(value().toDouble(), 0, 'g', m_precision + 1);
    return value().toString();
}
const QString &MandalaTreeField::units() const
{
    return m_units;
}
const QString &MandalaTreeField::caption() const
{
    return m_caption;
}
int MandalaTreeField::precision() const
{
    return m_precision;
}
QString MandalaTreeField::descr(void) const
{
    if (m_alias.isEmpty())
        return MandalaTree::descr();
    return QString("%1 (%2)").arg(MandalaTree::descr()).arg(m_alias);
}
//=============================================================================
//=============================================================================
