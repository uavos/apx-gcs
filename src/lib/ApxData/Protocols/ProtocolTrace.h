/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QtCore>

class ProtocolBase;

class ProtocolTraceItem
{
    Q_GADGET

public:
    enum TraceType {
        PACKET = 0,
        ERROR,
        PID,
        DATA,
        SQUAWK,
        NMT,
        GUID,
    };
    Q_ENUM(TraceType)

    TraceType m_type;
    QString m_text;

    Q_PROPERTY(TraceType type MEMBER m_type CONSTANT)
    Q_PROPERTY(QString text MEMBER m_text CONSTANT)
};

class ProtocolTrace : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolTrace(QObject *parent);

    friend class ProtocolBase;

    static void trace(bool uplink, ProtocolTraceItem::TraceType type, const QString &text);

private:
    static ProtocolTrace *_instance;
    QVariantList m_packet;
    bool m_uplink{false};

    void _trace(bool uplink, ProtocolTraceItem::TraceType type, const QString &text);
    void flush();

    QTimer flushTimer;

signals:
    void packet(bool uplink, QVariantList packet);
};

Q_DECLARE_METATYPE(ProtocolTraceItem)
