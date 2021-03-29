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

#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>

#include <App/AppNotify.h>
#include <Fact/Fact.h>
#include <Mandala/Mandala.h>

#include "Vehicles.h"

class Vehicles;
class Nodes;
class VehicleMission;
class VehicleWarnings;
class Telemetry;

class Vehicle : public Fact
{
    Q_OBJECT
    Q_ENUMS(FlightState)

    Q_PROPERTY(QString info READ toolTip NOTIFY infoChanged)

    Q_PROPERTY(bool follow READ follow WRITE setFollow NOTIFY followChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

    Q_PROPERTY(FlightState flightState READ flightState NOTIFY flightStateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(quint64 totalDistance READ totalDistance NOTIFY totalDistanceChanged)

    Q_PROPERTY(PVehicle *protocol READ protocol CONSTANT)
    Q_PROPERTY(PVehicle::StreamType streamType READ streamType NOTIFY streamTypeChanged)

    Q_PROPERTY(bool isLocal READ isLocal CONSTANT)
    Q_PROPERTY(bool isReplay READ isReplay CONSTANT)
    Q_PROPERTY(bool isIdentified READ isIdentified CONSTANT)
    Q_PROPERTY(bool isGroundControl READ isGroundControl NOTIFY isGroundControlChanged)

public:
    explicit Vehicle(Vehicles *vehicles, PVehicle *protocol);

    Mandala *f_mandala;
    Nodes *f_nodes;
    VehicleMission *f_mission;
    Telemetry *f_telemetry;
    VehicleWarnings *f_warnings;

    Fact *f_select;

    QTimer telemetryReqTimer;

    quint64 dbKey{0}; //from db

    QString fileTitle() const; //name based on Vehicle title and nodes shiva comment
    QString confTitle() const;

    Q_INVOKABLE QGeoRectangle geoPathRect() const;

    enum FlightState { FS_UNKNOWN = 0, FS_TAKEOFF, FS_LANDED };
    Q_ENUM(FlightState)

    QString uid() const { return _protocol ? _protocol->uid() : QString(); }
    PVehicle *protocol() const { return _protocol; }

private:
    PVehicle *_protocol;

    QTimer updateInfoTimer;

    Fact *f_lat;
    Fact *f_lon;
    Fact *f_hmsl;

    MandalaFact *f_pos;

    MandalaFact *f_ref;
    MandalaFact *f_ref_hmsl;

    Fact *f_vspeed;
    Fact *f_mode;
    Fact *f_stage;

private slots:
    void updateInfo();
    void updateInfoReq();
    void updateCoordinate();
    void updateFlightState();
    void updateGeoPath();

    void dbSetVehicleKey(quint64 key);

private slots:
    void updateActive();

signals:
    void selected();

signals:
    //forward from protocols
    void telemetryData();

    //forward for recorder
    void recordDownlink();
    void recordUplink(xbus::pid_s pid, QVariant value);

    //events
    void recordNodeMessage(QString subsystem, QString text, QString sn);
    void recordConfigUpdate(QString nodeName, QString fieldName, QString value, QString sn);
    void recordSerialData(quint16 portNo, QByteArray data, bool uplink);
    void recordConfig();

    void geoPathAppend(QGeoCoordinate p);

    //provided methods
public slots:
    void flyHere(const QGeoCoordinate &c);
    void lookHere(const QGeoCoordinate &c);
    void setHomePoint(const QGeoCoordinate &c);
    void sendPositionFix(const QGeoCoordinate &c);

    void resetGeoPath();

    void message(QString msg,
                 AppNotify::NotifyFlags flags = AppNotify::FromApp | AppNotify::Info,
                 QString subsystem = QString());

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

    bool isLocal() const { return m_is_local; }
    bool isReplay() const { return m_is_replay; }
    bool isIdentified() const { return m_is_identified; }
    bool isGroundControl() const { return m_is_gcs; }

    PVehicle::StreamType streamType() const;

protected:
    QString m_info;

    bool m_follow{false};
    QGeoCoordinate m_coordinate;
    FlightState m_flightState{FS_UNKNOWN};
    QGeoPath m_geoPath;
    quint64 m_totalDistance{0};

    bool m_is_local{};
    bool m_is_replay{};
    bool m_is_identified{};
    bool m_is_gcs{};

signals:
    void infoChanged();
    void followChanged();
    void coordinateChanged();
    void flightStateChanged();
    void geoPathChanged();
    void totalDistanceChanged();
    void isGroundControlChanged();
    void streamTypeChanged();
};
