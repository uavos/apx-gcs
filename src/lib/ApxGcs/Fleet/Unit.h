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

#include "UnitShare.h"
#include "UnitStorage.h"

#include "Fleet.h"

class Fleet;
class Nodes;
class UnitMission;
class UnitWarnings;
class Telemetry;

class Unit : public Fact
{
    Q_OBJECT
    Q_ENUMS(FlightState)

    Q_PROPERTY(QString info READ info NOTIFY infoChanged)

    Q_PROPERTY(bool follow READ follow WRITE setFollow NOTIFY followChanged)

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

    Q_PROPERTY(FlightState flightState READ flightState NOTIFY flightStateChanged)

    Q_PROPERTY(QGeoPath geoPath READ geoPath NOTIFY geoPathChanged)
    Q_PROPERTY(quint64 totalDistance READ totalDistance NOTIFY totalDistanceChanged)

    Q_PROPERTY(PUnit *protocol READ protocol CONSTANT)
    Q_PROPERTY(PUnit::StreamType streamType READ streamType NOTIFY streamTypeChanged)

    Q_PROPERTY(bool isLocal READ isLocal CONSTANT)
    Q_PROPERTY(bool isReplay READ isReplay CONSTANT)
    Q_PROPERTY(bool isIdentified READ isIdentified CONSTANT)
    Q_PROPERTY(bool isGroundControl READ isGroundControl NOTIFY isGroundControlChanged)

public:
    explicit Unit(Fleet *fleet, PUnit *protocol);

    Mandala *f_mandala;
    Nodes *f_nodes;
    UnitMission *f_mission;
    Telemetry *f_telemetry;
    UnitWarnings *f_warnings;

    Fact *f_select;
    // LookupUnitConfig *f_lookup;
    UnitStorage *f_storage;
    UnitShare *f_share;

    QTimer telemetryReqTimer;

    QString fileTitle() const; //name based on Unit title and nodes shiva comment
    QString confTitle() const;

    Q_INVOKABLE QGeoRectangle geoPathRect() const;

    enum FlightState { FS_UNKNOWN = 0, FS_TAKEOFF, FS_LANDED };
    Q_ENUM(FlightState)

    auto uid() const { return _protocol ? _protocol->uid() : QString(); }
    auto protocol() const { return _protocol; }

    QString unitTypeText() const { return _protocol ? _protocol->unitTypeText() : ""; }

    // variant conversions
    QJsonObject get_info() const;

    //Fact override
    QJsonValue toJson() override;
    void fromJson(const QJsonValue &jsv) override;

public:
    QString toolTip() const override;

    auto info() const { return m_info; }

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

    PUnit::StreamType streamType() const;

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

private:
    PUnit *_protocol;

    qint64 _lastSeenTime{};
    QJsonObject _importedUnitInfo; // stored for offline units (REPLAY)

    QTimer updateInfoTimer;

    Fact *f_lat;
    Fact *f_lon;
    Fact *f_hmsl;

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

    void updateActive();

    void packetReceived(mandala::uid_t uid);

    //provided methods
public slots:
    void flyHere(QGeoCoordinate c);
    void lookHere(QGeoCoordinate c);
    void setHomePoint(QGeoCoordinate c);
    void sendPositionFix(QGeoCoordinate c);

    void resetGeoPath();

    void message(QString msg,
                 AppNotify::NotifyFlags flags = AppNotify::FromApp | AppNotify::Info,
                 QString subsystem = QString(),
                 QString src_uid = QString());

signals:
    void selected();

    void requestScript(QString func, QVariant arg = {});
    void geoPathAppend(QGeoCoordinate p);
    void deleteUnit();

    void messageReported(QString msg, QString subsystem, QString src_uid);

    void infoChanged();
    void followChanged();
    void coordinateChanged();
    void flightStateChanged();
    void geoPathChanged();
    void totalDistanceChanged();
    void isGroundControlChanged();
    void streamTypeChanged();
};
