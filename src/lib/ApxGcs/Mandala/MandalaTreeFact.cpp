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
#include <QColor>

MandalaTreeFact::MandalaTreeFact(MandalaTree *tree, Fact *parent, const mandala::meta_t &meta)
    : Fact(parent, meta.name, meta.title, meta.title, meta.group ? Group : NoFlags)
    , m_tree(tree)
    , m_meta(meta)
{
    if (meta.level > 2)
        setOption(FilterSearchAll);
    if (meta.group) {
        updateStatus();
        connect(this, &Fact::sizeChanged, this, &MandalaTreeFact::updateStatus);
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
            //setPrecision(meta.prec);
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
            setUnits("enum");
            break;
        }
        if (!units().isEmpty()) {
            setDescr(QString("%1 [%2]").arg(descr()).arg(units()));
        }
        m_stream = get_stream();

        setValueLocal(v);
        sendTime.start();
        sendTimer.setInterval(100);
        sendTimer.setSingleShot(true);
        connect(&sendTimer, &QTimer::timeout, this, &MandalaTreeFact::send);

        connect(this, &Fact::valueChanged, this, &MandalaTreeFact::updateDescr);

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

mandala::uid_t MandalaTreeFact::uid()
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
    case Qt::ForegroundRole:
        if (col != FACT_MODEL_COLUMN_NAME)
            break;
        if (m_meta.group) {
            switch (m_meta.level) {
            case 0:
                return QColor(Qt::white);
            case 1:
                return QColor(Qt::blue).lighter(180);
            case 2:
                return QColor(Qt::yellow).lighter(180);
            case 3:
                return QColor(Qt::red).lighter(180);
            case 4:
                return QColor(Qt::red).lighter(180);
            }
        }
        break;
    case Qt::BackgroundRole:
        if (m_meta.level > 0)
            break;
        return QColor(Qt::darkCyan).darker(300);
    }
    return Fact::data(col, role);
}

bool MandalaTreeFact::showThis(QRegExp re) const
{
    if (Fact::showThis(re))
        return true;
    if (path(m_meta.level).contains(re))
        return true;
    return false;
}

void MandalaTreeFact::updateStatus()
{
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

    setDescr(s);
}

MandalaTreeStream *MandalaTreeFact::get_stream()
{
    /*switch (m_meta.sfmt) {
    case mandala::sfmt_u4:
        sfmt_text = "u4";
        return get_stream<mandala::sfmt_u4>(m_meta.type_id);
    case mandala::sfmt_u2:
        sfmt_text = "u2";
        return get_stream<mandala::sfmt_u2>(m_meta.type_id);
    case mandala::sfmt_u1:
        sfmt_text = "u1";
        return get_stream<mandala::sfmt_u1>(m_meta.type_id);
    case mandala::sfmt_f4:
        sfmt_text = "f4";
        return get_stream<mandala::sfmt_f4>(m_meta.type_id);
    case mandala::sfmt_f2:
        sfmt_text = "f2";
        return get_stream<mandala::sfmt_f2>(m_meta.type_id);
    case mandala::sfmt_f1:
        sfmt_text = "f1";
        return get_stream<mandala::sfmt_f1>(m_meta.type_id);
    case mandala::sfmt_f1_10:
        sfmt_text = "f1/10";
        return get_stream<mandala::sfmt_f1_10>(m_meta.type_id);
    case mandala::sfmt_f1_01:
        sfmt_text = "f1*10";
        return get_stream<mandala::sfmt_f1_01>(m_meta.type_id);
    case mandala::sfmt_f1_001:
        sfmt_text = "f1*100";
        return get_stream<mandala::sfmt_f1_001>(m_meta.type_id);
    case mandala::sfmt_f1_s:
        sfmt_text = "f1s";
        return get_stream<mandala::sfmt_f1_s>(m_meta.type_id);
    case mandala::sfmt_f1_s10:
        sfmt_text = "f1s/10";
        return get_stream<mandala::sfmt_f1_s10>(m_meta.type_id);
    case mandala::sfmt_f1_s01:
        sfmt_text = "f1s*10";
        return get_stream<mandala::sfmt_f1_s01>(m_meta.type_id);
    case mandala::sfmt_f1_s001:
        sfmt_text = "f1s*100";
        return get_stream<mandala::sfmt_f1_s001>(m_meta.type_id);
    }*/
    return nullptr;
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
