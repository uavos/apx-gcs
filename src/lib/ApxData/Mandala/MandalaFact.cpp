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
#include "MandalaFact.h"
#include "Mandala.h"
#include <App/AppLog.h>
#include <Mandala/MandalaMeta.h>
#include <Protocols/ProtocolStream.h>
#include <QColor>

MandalaFact::MandalaFact(Mandala *tree, Fact *parent, const mandala::meta_t &meta)
    : Fact(parent, meta.name, meta.title, "", meta.group ? Group | FilterModel : ModifiedTrack)
    , MandalaFactStream(meta.type_id)
    , m_tree(tree)
    , m_meta(meta)
{
    if (name().toLower() != name()) {
        apxMsgW() << "uppercase:" << name();
    }
    if (meta.level > 2)
        setOption(FilterSearchAll);
    if (isSystem()) {
        setOption(FilterExclude);
    }

    setDescr(mpath());

    if (meta.group) {
        /*connect(this, &Fact::sizeChanged, this, &Fact::valueChanged);
        if (meta.level == 2) {
            connect(this, &Fact::sectionChanged, parent, &Fact::textChanged);
        }*/
    } else {
        setUnits(meta.units);
        switch (meta.type_id) {
        case mandala::type_void:
            apxMsgW() << "void:" << mpath();
            break;
        case mandala::type_real:
            setDataType(Float);
            setPrecision(getPrecision());
            break;
        case mandala::type_dword:
            setDataType(Int);
            setMin(0);
            setMax(QVariant::fromValue(0xFFFFFFFFll));
            break;
        case mandala::type_word:
            setDataType(Int);
            setMin(0);
            setMax(QVariant::fromValue(0xFFFFu));
            break;
        case mandala::type_byte:
            setDataType(Int);
            setMin(0);
            setMax(255);
            break;
        case mandala::type_option: {
            setDataType(Enum);
            QStringList st = units().split(',');
            setUnits(QString());
            setEnumStrings(st);
            for (int i = 0; i < st.size(); ++i) {
                const QString &s
                    = QString("%1_%2_%3").arg(parentFact()->name()).arg(name()).arg(st.at(i));
                //const QString &s = QString("%1_%2").arg(name()).arg(st.at(i));
                if (!tree->constants.contains(s)) {
                    tree->constants.insert(s, i);
                    continue;
                }
                if (tree->constants.value(s) == i)
                    continue;
                apxMsgW() << "enum:" << s << mpath();
            }
        } break;
        }

        if (!isSystem()) {
            setOpt("color", getColor());
            sendTime.start();
            sendTimer.setInterval(100);
            sendTimer.setSingleShot(true);
            connect(&sendTimer, &QTimer::timeout, this, &MandalaFact::send);
            connect(this, &MandalaFact::sendUplink, tree, &Mandala::sendUplink);
        }
        connect(this, &Fact::triggered, this, [this]() { setModified(false); });
    }
}

void MandalaFact::addAlias(const QString &a)
{
    m_alias = a;
}

QString MandalaFact::alias() const
{
    return m_alias;
}

bool MandalaFact::setValue(const QVariant &v)
{
    //always send uplink
    bool rv = Fact::setValue(v);
    if (!rv)
        return false;

    //qDebug() << name() << text() << rv;
    if (sendTimer.isActive())
        return rv;
    if (sendTime.elapsed() >= sendTimer.interval())
        send();
    else
        sendTimer.start();
    return rv;
}

bool MandalaFact::setValueLocal(const QVariant &v)
{
    //don't send uplink
    return Fact::setValue(v);
}

void MandalaFact::setValueFromStream(const QVariant &v)
{
    qDebug() << v;
    setValueLocal(v);
}
QVariant MandalaFact::getValueForStream() const
{
    return value();
}

QByteArray MandalaFact::pack() const
{
    uint8_t buf[32];
    ProtocolStreamWriter stream(buf, sizeof(buf));
    *this >> stream;
    return stream.toByteArray();
}

bool MandalaFact::setValues(const QVariantList &vlist)
{
    if (!dataType())
        return false;
    MandalaFact *f = this;
    bool rv = false;
    QByteArray data;
    for (auto const &v : vlist) {
        if (!f)
            return false;
        if (f->setValueLocal(v))
            rv = true;
        data.append(f->pack());
        f = m_tree->fact(f->uid() + 1);
    }
    sendPacket(data);
    return rv;
}

void MandalaFact::sendPacket(const QByteArray data)
{
    uint8_t buf[32];
    ProtocolStreamWriter stream(buf, sizeof(buf));
    stream.write<xbus::pid_t>(uid());
    sendTime.start();
    emit sendUplink(stream.toByteArray().append(data));
}

mandala::uid_t MandalaFact::uid() const
{
    return m_meta.uid;
}
mandala::uid_t MandalaFact::offset() const
{
    return m_meta.uid - mandala::uid_base;
}
void MandalaFact::request()
{
    sendPacket(QByteArray());
}
void MandalaFact::send()
{
    sendPacket(pack());
}

QVariant MandalaFact::data(int col, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (col == FACT_MODEL_COLUMN_NAME)
            return name();
        if (col == FACT_MODEL_COLUMN_DESCR) {
            QString s = title();
            if (!alias().isEmpty())
                s += QString(" {%1}").arg(alias());
            if (!m_meta.descr[0])
                s += QString(" %1").arg(m_meta.descr);
            return s;
        }
        if (col == FACT_MODEL_COLUMN_VALUE) {
            if (!m_meta.group)
                break;
            if (m_meta.level == 0) {
                QStringList slist;
                QList<int> vlist;
                for (auto i : children()) {
                    const QString &s = i->section();
                    if (!slist.contains(s))
                        slist.append(s);
                    if (vlist.size() < slist.size())
                        vlist.append(0);
                    int idx = slist.indexOf(s);
                    vlist[idx] = vlist.at(idx) + 1;
                }
                QStringList st;
                for (auto n : vlist)
                    st.append(QString::number(n));
                int capacity = 1 << mandala::uid_bits[m_meta.level + 2];
                return QString("[%1/%2]").arg(st.join('/')).arg(capacity);
            }
            if (m_meta.level == 2) {
                int capacity = 1 << mandala::uid_bits[m_meta.level + 1];
                return QString("[%1/%2]").arg(size()).arg(capacity);
            }
        }
        break;
    case Qt::BackgroundRole:
        if (m_meta.level == 0)
            return QColor(Qt::darkCyan).darker(300);
        if (m_meta.level == 2 && isSystem())
            return QColor(Qt::darkRed).darker(300);

        /*if (col == FACT_MODEL_COLUMN_DESCR && opts().contains("color")) {
            return opts().value("color").value<QColor>();
        }*/
        break;
    }
    return Fact::data(col, role);
}

bool MandalaFact::showThis(QRegExp re) const
{
    if (Fact::showThis(re))
        return true;
    if (options() & FilterExclude)
        return false;
    if (!(options() & FilterSearchAll))
        return false;
    if (alias().contains(re))
        return true;
    if (mpath().contains(re))
        return true;
    if (m_meta.descr[0] && QString(m_meta.descr).contains(re))
        return true;
    return false;
}

Fact *MandalaFact::classFact() const
{
    int level = m_meta.level;
    if (level >= 1)
        level--;
    const Fact *f = this;
    for (int i = 0; i < level; ++i) {
        f = f->parentFact();
    }
    return const_cast<Fact *>(f);
}

QString MandalaFact::mpath() const
{
    int level = m_meta.level;
    if (level >= 1)
        level--;
    return path(level);
}

int MandalaFact::getPrecision()
{
    if (name().contains("lat") || name().contains("lon"))
        return 6;
    const QString &u = units().toLower();
    if (!u.isEmpty()) {
        if (u == "u")
            return 2;
        if (u == "su")
            return 2;
        if (u == "deg")
            return 1;
        if (u == "deg/s")
            return 1;
        if (u == "m")
            return 2;
        if (u == "m/s")
            return 1;
        if (u == "m/s2")
            return 1;
        if (u == "v")
            return 2;
        if (u == "a")
            return 3;
        if (u == "c")
            return 1;
        if (u == "1/min")
            return 0;
        if (u == "kpa")
            return 1;
        if (u == "bar")
            return 1;
        if (u == "l/d")
            return 2;
        if (u == "l/h")
            return 2;
        if (u == "l")
            return 2;
        if (u == "a/h")
            return 2;
        if (u == "pa")
            return 1;
        if (u == "n")
            return 1;
        if (u == "nm")
            return 1;
        if (u == "k")
            return 2;
        qWarning() << "default units precision:" << u << path();
    }
    return 6;
}

QColor MandalaFact::getColor()
{
    QString sclass = classFact()->name();
    QString sub = parentFact()->name();
    QString sn = name();
    bool isEnum = !enumStrings().isEmpty();

    int vectidx = -1;
    int vectfactor = -1;
    QList<QStringList> vlist;
    vlist << (QStringList() << "roll"
                            << "pitch"
                            << "yaw");
    vlist << (QStringList() << "x"
                            << "y"
                            << "z");
    vlist << (QStringList() << "p"
                            << "q"
                            << "r");
    vlist << (QStringList() << "n"
                            << "e"
                            << "d");
    vlist << (QStringList() << "vn"
                            << "ve"
                            << "vd");
    vlist << (QStringList() << "lat"
                            << "lon"
                            << "hmsl");

    for (auto st : vlist) {
        vectfactor++;
        for (int i = 0; i < 3; ++i) {
            if (sn == st.at(i) || (sn.endsWith(st.at(i)))) {
                vectidx = i;
                break;
            }
        }
        if (vectidx >= 0)
            break;
    }

    QColor c(Qt::cyan);
    if (sclass == "ctr") {
        if (sn.contains("ail"))
            c = QColor(Qt::red).lighter();
        else if (sn.contains("elv"))
            c = QColor(Qt::green).lighter();
        else if (sn.contains("thr"))
            c = QColor(Qt::blue).lighter();
        else if (sn.contains("rud"))
            c = QColor(Qt::yellow).lighter();
        else if (sn.contains("col"))
            c = QColor(Qt::darkCyan);
        else if (isEnum)
            c = Qt::magenta;
        else
            c = QColor(Qt::magenta).darker();
    } else if (sub == "rc") {
        if (sn.contains("roll"))
            c = QColor(Qt::darkRed);
        else if (sn.contains("pitch"))
            c = QColor(Qt::darkGreen);
        else if (sn.contains("throttle"))
            c = QColor(Qt::darkBlue);
        else if (sn.contains("yaw"))
            c = QColor(Qt::darkYellow);
        else
            c = QColor(Qt::magenta).lighter();
    } else if (sub == "usr") {
        if (sn.endsWith("1"))
            c = QColor(Qt::red).lighter();
        else if (sn.endsWith("2"))
            c = QColor(Qt::green).lighter();
        else if (sn.endsWith("3"))
            c = QColor(Qt::blue).lighter();
        else if (sn.endsWith("4"))
            c = QColor(Qt::yellow).lighter();
        else if (sn.endsWith("5"))
            c = QColor(Qt::cyan).lighter();
        else if (sn.endsWith("6"))
            c = QColor(Qt::magenta).lighter();
        else
            c = QColor(Qt::cyan).lighter();
    } else if (sn.contains("altitude"))
        c = QColor(Qt::red).lighter(170);
    else if (sn.contains("vspeed"))
        c = QColor(Qt::green).lighter(170);
    else if (sn.contains("airspeed"))
        c = QColor(Qt::blue).lighter(170);
    else if (isEnum)
        c = QColor(Qt::blue).lighter();
    else if (vectidx >= 0) {
        if (vectidx == 0)
            c = Qt::red;
        else if (vectidx == 1)
            c = Qt::green;
        else if (vectidx == 2)
            c = Qt::yellow;
        if (vectfactor > 0)
            c = c.lighter(100 + vectfactor * 10);
        if (sclass == "cmd")
            c = c.lighter();
    }
    return c;
}

bool MandalaFact::isSystem() const
{
    return m_meta.uid
           >= (mandala::uid_base + (3 << mandala::uid_shift[0]) | (1 << mandala::uid_shift[1]));
}
