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
#include "MandalaTreeFact.h"
#include "MandalaTree.h"
#include <App/AppLog.h>
#include <Mandala/MandalaMeta.h>
#include <Mandala/MandalaStream.h>
#include <QColor>

MandalaTreeFact::MandalaTreeFact(MandalaTree *tree, Fact *parent, const mandala::meta_t &meta)
    : Fact(parent, meta.name, meta.title, meta.title, meta.group ? Group : NoFlags)
    , m_tree(tree)
    , m_meta(meta)
{
    if (meta.level > 2)
        setOption(FilterSearchAll);
    if (isSystem()) {
        setOption(FilterExclude);
    }
    if (meta.group) {
        updateStatus();
        connect(this, &Fact::sizeChanged, this, &MandalaTreeFact::updateStatus);
        if (meta.level == 0) {
            setOption(FlatModel);
        } else if (meta.level == 1) {
            setOption(Section);
        }
    } else {
        QString sdescr = meta.descr;
        if (!sdescr.isEmpty()) {
            setDescr(QString("%1 - %2").arg(descr()).arg(sdescr));
        }

        setUnits(meta.units);
        QVariant v;
        switch (meta.type_id) {
        case mandala::type_float:
            setDataType(Float);
            v = static_cast<double>(0.0);
            setPrecision(getPrecision());
            break;
        case mandala::type_dword:
            setDataType(Int);
            setMin(0);
            setMax(QVariant::fromValue(0xFFFFFFFFll));
            v = QVariant::fromValue(0);
            break;
        case mandala::type_word:
            setDataType(Int);
            setMin(0);
            setMax(QVariant::fromValue(0xFFFFu));
            v = QVariant::fromValue(0);
            break;
        case mandala::type_byte:
            setDataType(Int);
            setMin(0);
            setMax(255);
            v = QVariant::fromValue(0);
            break;
        case mandala::type_enum:
            setDataType(Enum);
            v = static_cast<int>(0);
            setEnumStrings(units().split(','));
            setUnits("");
            break;
        }
        if (!units().isEmpty()) {
            setDescr(QString("%1 [%2]").arg(descr()).arg(units()));
        }

        setValueLocal(v);

        if (!isSystem()) {
            m_stream = MandalaTreeStream::get_stream();
            setOpt("color", getColor());
            sendTime.start();
            sendTimer.setInterval(100);
            sendTimer.setSingleShot(true);
            connect(&sendTimer, &QTimer::timeout, this, &MandalaTreeFact::send);
            connect(this, &Fact::valueChanged, this, &MandalaTreeFact::updateDescr);
        }

        //integrity tests
        /* switch (meta.type_id) {
        case mandala::type_enum:
            if (meta.sfmt < mandala::sfmt_f4) {
                qWarning() << path() << "enum sfmt";
            }
            break;
        case mandala::type_byte:
            if (meta.sfmt < mandala::sfmt_f4) {
                qWarning() << path() << "byte sfmt";
            }
            break;
        case mandala::type_float:
            if (meta.sfmt < mandala::sfmt_f4) {
                qWarning() << path() << "float sfmt";
            }
            break;
        case mandala::type_uint:
            if (meta.sfmt >= mandala::sfmt_f4) {
                qWarning() << path() << "uint sfmt";
            }
            break;
        }*/
    }
    setDescr(QString("%1: %2").arg(meta.uid, 4, 16, QChar('0')).arg(descr()));
    updateDescr();
}

bool MandalaTreeFact::setValue(const QVariant &v)
{
    //always send uplink
    bool rv = Fact::setValue(v);
    //qDebug()<<name()<<text();
    if (sendTimer.isActive())
        return rv;
    if (sendTime.elapsed() >= sendTimer.interval())
        send();
    else
        sendTimer.start();
    return rv;
}

bool MandalaTreeFact::setValueLocal(const QVariant &v)
{
    //don't send uplink
    return Fact::setValue(v);
}

mandala::uid_t MandalaTreeFact::uid() const
{
    return m_meta.uid;
}
void MandalaTreeFact::request()
{
    emit sendValueRequest(uid());
}
void MandalaTreeFact::send()
{
    sendTime.start();
    emit sendValueUpdate(uid(), value().toDouble());
}

QVariant MandalaTreeFact::data(int col, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (col != FACT_MODEL_COLUMN_NAME)
            break;
        return name();
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

bool MandalaTreeFact::showThis(QRegExp re) const
{
    if (Fact::showThis(re))
        return true;
    if (options() & FilterExclude)
        return false;
    if (!(options() & FilterSearchAll))
        return false;
    if (path(m_meta.level).contains(re))
        return true;
    return false;
}

void MandalaTreeFact::updateStatus()
{
    if (m_meta.level < 2)
        return;
    int capacity = 1 << mandala::uid_bits[m_meta.level + 1];
    setStatus(QString("[%1/%2]").arg(size()).arg(capacity));
}

void MandalaTreeFact::updateDescr()
{
    QString s = m_meta.title;
    QString sdescr = m_meta.descr;
    if (!sdescr.isEmpty()) {
        s = QString("%1 - %2").arg(s).arg(sdescr);
    }
    if (!units().isEmpty()) {
        s = QString("%1 [%2]").arg(s).arg(units());
    }
    if (m_stream) {
        QByteArray ba(100, '\0');
        size_t sz = pack(ba.data());
        ba.resize(sz);
        //qDebug() << path() << ba.toHex().toUpper();
        s = QString("(%2) %1: %3").arg(QString(ba.toHex().toUpper())).arg(m_stream->psize()).arg(s);
        s = QString("[%1] %2").arg(m_stream->type_text).arg(s);
        s = QString("[%1] %2").arg(m_stream->sfmt_text).arg(s);
    }
    //s = QString("%1: %2").arg(m_meta.uid, 4, 16, QChar('0')).arg(s);
    if (precision() >= 0) {
        s = QString("[%1] %2").arg(precision()).arg(s);
    }

    setDescr(s);
}

size_t MandalaTreeFact::pack(void *buf) const
{
    if (!m_stream)
        return 0;
    return m_stream->pack(buf, value());
}

size_t MandalaTreeFact::unpack(const void *buf)
{
    if (!m_stream)
        return 0;
    QVariant v;
    size_t sz = m_stream->unpack(buf, v);
    if (sz > 0)
        setValueLocal(v);
    return sz;
}

Fact *MandalaTreeFact::classFact() const
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

QString MandalaTreeFact::mpath() const
{
    int level = m_meta.level;
    if (level >= 1)
        level--;
    return path(level);
}

int MandalaTreeFact::getPrecision()
{
    if (name().contains("lat") || name().contains("lon"))
        return 8;
    const QString &u = units().toLower();
    if (!u.isEmpty()) {
        if (u == "u")
            return 3;
        if (u == "su")
            return 3;
        if (u == "deg")
            return 2;
        if (u == "deg/s")
            return 2;
        if (u == "m")
            return 2;
        if (u == "m/s")
            return 2;
        if (u == "m/s2")
            return 2;
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

QColor MandalaTreeFact::getColor()
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
            if (sn == st.at(i) || (sn.contains('_') && sn.endsWith(st.at(i)))) {
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
    } else if (sn.startsWith("altitude"))
        c = QColor(Qt::red).lighter(170);
    else if (sn.startsWith("vspeed"))
        c = QColor(Qt::green).lighter(170);
    else if (sn.startsWith("airspeed"))
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

QString MandalaTreeFact::alias() const
{
    return m_alias;
}

bool MandalaTreeFact::isSystem() const
{
    return m_meta.uid
           >= (mandala::uid_base + (3 << mandala::uid_shift[0]) | (1 << mandala::uid_shift[1]));
}
