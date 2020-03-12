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

#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>

#include <App/AppNotify.h>
#include <Fact/Fact.h>
#include <Mandala/Mandala.h>

#include <Protocols/ProtocolVehicle.h>
#include <Protocols/ProtocolViewBase.h>

class Vehicles;
class Nodes;
class VehicleMission;
class VehicleWarnings;
class Telemetry;

class Vehicle : public ProtocolViewBase<ProtocolVehicle>
{
    Q_OBJECT
    Q_ENUMS(StreamType)
    Q_ENUMS(FlightState)

    Q_PROPERTY(quint16 squawk READ squawk NOTIFY squawkChanged)

    Q_PROPERTY(StreamType streamType READ streamType NOTIFY streamTypeChanged)
    Q_PROPERTY(QString streamTypeText READ streamTypeText NOTIFY streamTypeChanged)

    Q_PROPERTY(QString info READ info NOTIFY infoChanged)

    Q_PROPERTY(bool follow READ follow WRITE setFollow NOTIFY followChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

    Q_PROPERTY(FlightState flightState READ flightState NOTIFY flightStateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(quint64 totalDistance READ totalDistance NOTIFY totalDistanceChanged)

    Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)

public:
    enum StreamType { OFFLINE = 0, SERVICE, DATA, XPDR, TELEMETRY };
    Q_ENUM(StreamType)

    enum FlightState { FS_UNKNOWN = 0, FS_TAKEOFF, FS_LANDED };
    Q_ENUM(FlightState)

    explicit Vehicle(Vehicles *vehicles, ProtocolVehicle *protocol);

    ~Vehicle() override;

    Mandala *f_mandala;
    Nodes *f_nodes;
    VehicleMission *f_mission;
    Telemetry *f_telemetry;
    VehicleWarnings *f_warnings;

    Fact *f_select;

    QString uid;

    quint64 dbKey{0}; //from db

    QString streamTypeText() const;
    QString squawkText() const;
    QString squawkText(quint16 v) const;

    QString fileTitle() const; //name based on Vehicle title and nodes shiva comment
    QString confTitle() const;

    Q_INVOKABLE bool isLocal() const;
    Q_INVOKABLE bool isReplay() const;

    Q_INVOKABLE QGeoRectangle geoPathRect() const;

private:
    QTimer telemetryReqTimer;
    QTimer onlineTimer;
    QElapsedTimer telemetryTime;
    QElapsedTimer xpdrTime;
    QElapsedTimer replayTime;

    QTimer updateInfoTimer;

    Fact *f_lat;
    Fact *f_lon;
    Fact *f_hmsl;

    MandalaFact *f_ref_lat;
    MandalaFact *f_ref_lon;
    MandalaFact *f_ref_hmsl;

    Fact *f_vd;
    Fact *f_mode;
    Fact *f_stage;

    MandalaFact *f_cmd_n;
    MandalaFact *f_cmd_e;
    MandalaFact *f_cmd_gimbal_lat;
    MandalaFact *f_cmd_gimbal_lon;
    MandalaFact *f_cmd_gimbal_hmsl;

    MandalaFact *f_gps_lat;
    MandalaFact *f_gps_lon;
    MandalaFact *f_gps_hmsl;

    void setReplay(bool v);

private slots:
    void updateStatus();
    void updateInfo();
    void updateInfoReq();
    void updateCoordinate();
    void updateFlightState();
    void updateGeoPath();

    void dbSetVehicleKey(quint64 key);

private slots:
    //protocols connection
    void setStreamXpdr();
    void setStreamTelemetry();
    void setStreamData();
    void setStreamService();

    void updateActive();

    void updateDatalinkVars(quint16 id, QByteArray);

    void jsexecData(QString data);

signals:
    void selected();

signals:
    //forward for recorder
    void recordDownlink();
    void recordUplink(Fact *f);
    //events
    void recordNodeMessage(QString nodeName, QString text, QString sn);
    void recordConfigUpdate(QString nodeName, QString fieldName, QString value, QString sn);
    void recordSerialData(quint16 portNo, QByteArray data, bool uplink);

    void geoPathAppend(QGeoCoordinate p);

    //provided methods
public slots:
    void vmexec(QString func);
    void sendSerial(quint8 portID, QByteArray data);

    void flyHere(const QGeoCoordinate &c);
    void lookHere(const QGeoCoordinate &c);
    void setHomePoint(const QGeoCoordinate &c);
    void sendPositionFix(const QGeoCoordinate &c);

    void resetGeoPath();

    void message(QString msg,
                 AppNotify::NotifyFlags flags = AppNotify::FromApp | AppNotify::Info,
                 QString subsystem = QString());

    //Database
public slots:
    void dbSaveVehicleInfo();

    //---------------------------------------
    // PROPERTIES
public:
    quint16 squawk(void) const;

    StreamType streamType(void) const;
    void setStreamType(const StreamType v);

    QString info(void) const override;

    bool follow(void) const;
    void setFollow(const bool &v);

    QGeoCoordinate coordinate(void) const;
    void setCoordinate(const QGeoCoordinate &v);

    FlightState flightState(void) const;
    void setFlightState(const FlightState &v);

    QGeoPath geoPath() const;
    void setGeoPath(const QGeoPath &v);

    quint64 totalDistance() const;
    void setTotalDistance(quint64 v);

    uint errcnt(void) const;
    bool setErrcnt(const uint &v);

protected:
    StreamType m_streamType{OFFLINE};
    QString m_info;

    bool m_follow{false};
    QGeoCoordinate m_coordinate;
    FlightState m_flightState{FS_UNKNOWN};
    QGeoPath m_geoPath;
    quint64 m_totalDistance{0};
    uint m_errcnt{0};

signals:
    void streamTypeChanged();
    void squawkChanged();
    void infoChanged();
    void followChanged();
    void coordinateChanged();
    void flightStateChanged();
    void geoPathChanged();
    void totalDistanceChanged();
    void errcntChanged();
};
