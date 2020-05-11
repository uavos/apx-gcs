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
#pragma once

#include "ProtocolBase.h"
#include "ProtocolNodes.h"
#include "ProtocolTelemetry.h"
#include "ProtocolVehicles.h"

#include <QtCore>

class ProtocolVehicle : public ProtocolBase
{
    Q_OBJECT
    Q_ENUMS(StreamType)
    Q_PROPERTY(quint16 squawk READ squawk NOTIFY squawkChanged)
    Q_PROPERTY(QString squawkText READ squawkText NOTIFY squawkChanged)

    Q_PROPERTY(StreamType streamType READ streamType NOTIFY streamTypeChanged)
    Q_PROPERTY(QString streamTypeText READ streamTypeText NOTIFY streamTypeChanged)

    Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)

    Q_PROPERTY(bool isLocal READ isLocal CONSTANT)
    Q_PROPERTY(bool isReplay READ isReplay CONSTANT)
    Q_PROPERTY(bool isIdentified READ isIdentified CONSTANT)
    Q_PROPERTY(bool isGroundControl READ isGroundControl NOTIFY identChanged)

public:
    ProtocolVehicle(ProtocolVehicles *vehicles,
                    xbus::vehicle::squawk_t squawk,
                    const xbus::vehicle::ident_s &ident,
                    const QString &callsign);

    friend class ProtocolVehicles;

    enum StreamType { OFFLINE = 0, NMT, DATA, XPDR, TELEMETRY };
    Q_ENUM(StreamType)

    // called by vehicles
    void downlink(ProtocolStreamReader &stream);

    const xbus::vehicle::ident_s &ident() const;

    void updateIdent(const xbus::vehicle::squawk_t &squawk,
                     const xbus::vehicle::ident_s &ident,
                     const QString &name);

    bool match(const xbus::vehicle::squawk_t &squawk) const;
    bool match(const xbus::vehicle::uid_t &uid) const;
    bool match(const xbus::vehicle::ident_s &ident) const;

    ProtocolNodes *nodes;
    ProtocolTelemetry *telemetry;
    ProtocolVehicles *vehicles;

private:
    xbus::vehicle::ident_s m_ident;

    uint8_t txbuf[xbus::size_packet_max];
    ProtocolStreamWriter ostream{txbuf, sizeof(txbuf)};

    QTimer onlineTimer;
    QElapsedTimer time_telemetry;
    QElapsedTimer time_xpdr;

    QTimer telemetryReqTimer;

private slots:
    void updateActive();

public slots:
    void inc_errcnt();

    void updateStreamType(StreamType type);

    void requestTelemetry();

    void send(const QByteArray packet);
    void vmexec(QString func);
    void sendSerial(quint8 portID, QByteArray data);

signals:
    void identChanged();

    void xpdrData(const xbus::vehicle::xpdr_s &xpdr);

    //known received data
    void serialRxData(quint16 portNo, QByteArray data);
    void serialTxData(quint16 portNo, QByteArray data);

    void jsexecData(QString script);
    void missionData(QByteArray data);

    //---------------------------------------
    // PROPERTIES
public:
    xbus::vehicle::squawk_t squawk() const;
    QString squawkText() const;

    StreamType streamType(void) const;
    QString streamTypeText() const;

    uint errcnt(void) const;
    void setErrcnt(const uint &v);

    bool isLocal() const;
    bool isReplay() const;
    bool isIdentified() const;
    bool isGroundControl() const;

protected:
    xbus::vehicle::squawk_t m_squawk{0};
    StreamType m_streamType{OFFLINE};
    uint m_errcnt{0};

signals:
    void squawkChanged();
    void streamTypeChanged();
    void errcntChanged();
};
