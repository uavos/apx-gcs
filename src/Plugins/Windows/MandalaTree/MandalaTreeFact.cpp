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
#include <MandalaMeta.h>
#include <QColor>

MandalaTreeFact::MandalaTreeFact(Fact *parent, const mandala::meta_t &meta)
    : Fact(parent, meta.name, meta.name, meta.title, meta.group ? Group : NoFlags)
    , m_meta(meta)
{
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
            break;
        case mandala::type_uint:
            setDataType(Int);
            setMin(0x00000000);
            setMax(0xFFFFFFFF);
            v = static_cast<int>(0);
            break;
        case mandala::type_byte:
            setDataType(Int);
            setMin(0x00);
            setMax(0xFF);
            v = static_cast<int>(0);
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

        setValueLocal(v);
        sendTime.start();
        sendTimer.setInterval(100);
        sendTimer.setSingleShot(true);
        connect(&sendTimer, &QTimer::timeout, this, &MandalaTreeFact::send);
    }
    setDescr(QString("%1: %2").arg(meta.uid, 4, 16, QChar('0')).arg(descr()));
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

quint16 MandalaTreeFact::id()
{
    return m_meta.uid;
}
void MandalaTreeFact::request()
{
    emit sendValueRequest(id());
}
void MandalaTreeFact::send()
{
    sendTime.start();
    emit sendValueUpdate(id(), value().toDouble());
}

QVariant MandalaTreeFact::data(int col, int role) const
{
    switch (role) {
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
    }
    return Fact::data(col, role);
}

void MandalaTreeFact::updateStatus()
{
    int capacity = 1 << mandala::uid_bits[m_meta.level + 1];
    setStatus(QString("[%1/%2]").arg(size()).arg(capacity));
}
