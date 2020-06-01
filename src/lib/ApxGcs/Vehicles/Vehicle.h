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
    Q_ENUMS(FlightState)

    Q_PROPERTY(QString info READ toolTip NOTIFY infoChanged)

    Q_PROPERTY(bool follow READ follow WRITE setFollow NOTIFY followChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

    Q_PROPERTY(FlightState flightState READ flightState NOTIFY flightStateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(quint64 totalDistance READ totalDistance NOTIFY totalDistanceChanged)

public:
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

    QString fileTitle() const; //name based on Vehicle title and nodes shiva comment
    QString confTitle() const;

    Q_INVOKABLE QGeoRectangle geoPathRect() const;

    enum FlightState { FS_UNKNOWN = 0, FS_TAKEOFF, FS_LANDED };
    Q_ENUM(FlightState)

private:
    QTimer updateInfoTimer;

    Fact *f_lat;
    Fact *f_lon;
    Fact *f_hmsl;

    MandalaFact *f_ref_lat;
    MandalaFact *f_ref_lon;
    MandalaFact *f_ref_hmsl;

    Fact *f_vspeed;
    Fact *f_mode;
    Fact *f_stage;

    MandalaFact *f_cmd_n;
    MandalaFact *f_cmd_e;
    MandalaFact *f_cmd_gimbal_lat;
    MandalaFact *f_cmd_gimbal_lon;
    MandalaFact *f_cmd_gimbal_hmsl;

private slots:
    void updateInfo();
    void updateInfoReq();
    void updateCoordinate();
    void updateFlightState();
    void updateGeoPath();

    void dbSetVehicleKey(quint64 key);

private slots:
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
    QString toolTip(void) const override;

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

protected:
    QString m_info;

    bool m_follow{false};
    QGeoCoordinate m_coordinate;
    FlightState m_flightState{FS_UNKNOWN};
    QGeoPath m_geoPath;
    quint64 m_totalDistance{0};

signals:
    void infoChanged();
    void followChanged();
    void coordinateChanged();
    void flightStateChanged();
    void geoPathChanged();
    void totalDistanceChanged();
};
