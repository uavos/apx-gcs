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
#include "Vehicle.h"
#include "VehicleWarnings.h"
#include "Vehicles.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <Database/VehiclesReqVehicle.h>
#include <Mandala/Mandala.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
#include <Telemetry/Telemetry.h>

Vehicle::Vehicle(Vehicles *vehicles, PVehicle *protocol)
    : Fact(vehicles, protocol ? protocol->name() : "replay", protocol ? protocol->title() : "REPLAY")
    , _protocol(protocol)
{
    _storage = new VehicleStorage(this);

    if (protocol) {
        bindProperty(protocol, "title", true);
        bindProperty(protocol, "value", true);
        bindProperty(protocol, "active");

        // vehicle deletion
        connect(this, &Fact::removed, protocol, &Fact::deleteFact);

        connect(protocol, &PVehicle::streamTypeChanged, this, &Vehicle::streamTypeChanged);

        if (protocol->uid().isEmpty()) {
            m_is_local = true;
            setName("local");
        } else {
            m_is_identified = true;
            m_is_gcs = protocol->vehicleType() == PVehicle::GCS;
            connect(protocol, &PVehicle::vehicleTypeChanged, this, [this]() {
                m_is_gcs = _protocol->vehicleType() == PVehicle::GCS;
                emit isGroundControlChanged();
            });
        }
    } else {
        m_is_replay = true;
    }

    setIcon(m_is_identified ? "drone" : "chip");

    f_select
        = new Fact(this, "select", tr("Select"), tr("Make this vehicle active"), Action, "select");
    connect(f_select, &Fact::triggered, this, [this, vehicles]() { vehicles->selectVehicle(this); });

    f_mandala = new Mandala(this);
    f_nodes = new Nodes(this);
    f_mission = new VehicleMission(this);
    f_warnings = new VehicleWarnings(this);

    if (isIdentified()) {
        auto f_remove = new Fact(this,
                                 "remove",
                                 tr("Remove"),
                                 tr("Remove vehicle"),
                                 Action | Remove | IconOnly);
        connect(f_remove, &Fact::triggered, this, &Vehicle::deleteVehicle);
        connect(this, &Vehicle::deleteVehicle, vehicles, [this, vehicles]() {
            vehicles->deleteVehicle(this);
        });
    }

    f_lookup = new LookupVehicleConfig(this, this);
    f_share = new VehicleShare(this, this);

    setMandala(f_mandala);

    //Mandala facts binfing
    f_lat = f_mandala->fact(mandala::est::nav::pos::lat::uid);
    f_lon = f_mandala->fact(mandala::est::nav::pos::lon::uid);
    f_hmsl = f_mandala->fact(mandala::est::nav::pos::hmsl::uid);

    f_ref_hmsl = f_mandala->fact(mandala::est::nav::ref::hmsl::uid);

    f_vspeed = f_mandala->fact(mandala::est::nav::pos::vspeed::uid);
    f_mode = f_mandala->fact(mandala::cmd::nav::proc::mode::uid);
    f_stage = f_mandala->fact(mandala::cmd::nav::proc::stage::uid);

    updateInfoTimer.setInterval(300);
    updateInfoTimer.setSingleShot(true);
    connect(&updateInfoTimer, &QTimer::timeout, this, &Vehicle::updateInfo);

    connect(f_lat, &Fact::valueChanged, this, &Vehicle::updateCoordinate);
    connect(f_lon, &Fact::valueChanged, this, &Vehicle::updateCoordinate);
    connect(f_hmsl, &Fact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_vspeed, &Fact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_mode, &Fact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_stage, &Fact::valueChanged, this, &Vehicle::updateInfoReq);

    connect(f_mode, &Fact::valueChanged, this, &Vehicle::updateFlightState);
    connect(f_stage, &Fact::valueChanged, this, &Vehicle::updateFlightState);

    connect(this, &Fact::activeChanged, this, &Vehicle::updateActive);
    connect(f_nodes, &Nodes::upgradingChanged, this, &Vehicle::updateActive);

    if (protocol) {
        connect(this,
                &Vehicle::coordinateChanged,
                this,
                &Vehicle::updateGeoPath,
                Qt::QueuedConnection);

        // mandala update
        connect(f_mandala, &Mandala::sendValue, protocol->data(), &PData::sendValue);
        connect(protocol->data(), &PData::valuesData, f_mandala, &Mandala::valuesData);

        connect(protocol->telemetry(),
                &PTelemetry::telemetryData,
                f_mandala,
                &Mandala::telemetryData);
        connect(protocol->telemetry(),
                &PTelemetry::telemetryReset,
                f_mandala,
                &Mandala::resetCounters);

        connect(protocol->telemetry(), &PTelemetry::xpdrData, f_mandala, &Mandala::telemetryData);

        connect(protocol->data(), &PData::jsexecData, App::instance(), &App::jsexec);

        // storage
        connect(this, &Vehicle::titleChanged, _storage, &VehicleStorage::saveVehicleInfo);
        connect(this, &Vehicle::isGroundControlChanged, _storage, &VehicleStorage::saveVehicleInfo);
        _storage->saveVehicleInfo();

        // counters
        connect(protocol, &PVehicle::packetReceived, this, &Vehicle::packetReceived);

        // forward serial TX for plugins
        connect(this, &Vehicle::requestScript, protocol->data(), &PData::requestScript);
    }

    if (isIdentified()) {
        _lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

        telemetryReqTimer.setInterval(1000);
        connect(&telemetryReqTimer,
                &QTimer::timeout,
                protocol->telemetry(),
                &PTelemetry::requestTelemetry);
    }

    // telemetry data recorder/player
    // will connect to protocol by itself and must be created after vehicle's protocol connections
    f_telemetry = new Telemetry(this);

    // path
    Fact *f = new Fact(f_telemetry,
                       "rpath",
                       tr("Reset Path"),
                       tr("Clear travelled path"),
                       Action,
                       "history");
    connect(f, &Fact::triggered, this, &Vehicle::resetGeoPath);
    connect(this, &Vehicle::geoPathChanged, f, [this, f]() { f->setEnabled(!geoPath().isEmpty()); });
    f->setEnabled(false);

    setOpt("VID", uid());

    updateInfo();

    App::jsync(this);
}

void Vehicle::packetReceived(mandala::uid_t uid)
{
    if (mandala::cmd::env::match(uid)) {
        MandalaFact *f = f_mandala->fact(uid);
        if (f)
            f->increment_rx_cnt();
    }
}

void Vehicle::updateActive()
{
    bool sel = active();

    f_select->setEnabled(!sel);
    if (sel)
        emit selected();
    setFollow(false);

    if (isIdentified()) {
        if (sel && !f_nodes->upgrading())
            telemetryReqTimer.start();
        else
            telemetryReqTimer.stop();
    }
}

QVariantMap Vehicle::get_info() const
{
    if (!_protocol)
        return {};

    QVariantMap vehicle;
    vehicle.insert("uid", _protocol->uid());
    vehicle.insert("callsign", title());
    vehicle.insert("class", _protocol->vehicleTypeText());
    if (_lastSeenTime)
        vehicle.insert("time", _lastSeenTime);
    return vehicle;
}
QVariant Vehicle::toVariant()
{
    QVariantMap m;

    if (_protocol)
        m.insert("vehicle", get_info());

    m.insert("nodes", f_nodes->toVariant());
    m.insert("title", f_nodes->getConfigTitle());
    m.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());

    return m;
}
void Vehicle::fromVariant(const QVariant &var)
{
    auto m = var.value<QVariantMap>();
    if (m.isEmpty())
        return;

    auto nodes = m.value("nodes").value<QVariantList>();
    if (nodes.isEmpty()) {
        apxMsgW() << tr("Missing nodes in data set");
    } else {
        f_nodes->fromVariant(nodes);
    }
}

void Vehicle::updateInfo()
{
    if (isIdentified()) {
        _lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }
    QStringList st;
    //st<<callsign();
    if (!isReplay()) {
        QString s;
        int alt = f_hmsl->value().toInt();
        if (std::abs(alt) >= 50)
            alt = (alt / 10) * 10;
        else if (alt < 1)
            alt = 0;
        s = QString("MSL%1").arg(alt);

        int vs = f_vspeed->value().toInt();
        if (vs > 1)
            s.append(QString("+%1").arg(vs));
        else if (vs < -1)
            s.append(QString("%1").arg(vs));
        st << s;
        s = f_mode->text();
        uint stage = f_stage->value().toUInt();
        if (stage > 1)
            s.append(QString("/%1").arg(stage));
        st << s;
    }

    QString v = st.join("\n");
    if (m_info == v)
        return;
    m_info = v;
    emit infoChanged();
}
void Vehicle::updateInfoReq()
{
    if (updateInfoTimer.isActive())
        return;
    updateInfoTimer.start();
}
void Vehicle::updateCoordinate()
{
    setCoordinate(QGeoCoordinate(f_lat->value().toDouble(),
                                 f_lon->value().toDouble(),
                                 f_hmsl->value().toDouble()));
}
void Vehicle::updateFlightState()
{
    if ((f_mode->value().toUInt() == mandala::proc_mode_LANDING)
        && (f_stage->value().toUInt() >= 7)) {
        //setFlightState(FS_LANDED);
        // TODO improve landed condition detector, independently checking the state of the vehicle
    } else if ((f_mode->value().toUInt() == mandala::proc_mode_TAKEOFF)
               && (f_stage->value().toUInt() >= 2) && (f_stage->value().toUInt() < 100)) {
        setFlightState(FS_TAKEOFF);
    } else
        setFlightState(FS_UNKNOWN);
}
void Vehicle::updateGeoPath()
{
    QGeoCoordinate c(coordinate());
    if (!c.isValid())
        return;
    if (c.latitude() == 0.0)
        return;
    if (c.longitude() == 0.0)
        return;
    if (!m_geoPath.isEmpty()) {
        QGeoCoordinate c0(m_geoPath.path().last());
        /*if (c0.latitude() == c.latitude())
            return;
        if (c0.longitude() == c.longitude())
            return;*/
        quint64 d = static_cast<quint64>(c0.distanceTo(c));
        if (d < 10)
            return;
        setTotalDistance(totalDistance() + static_cast<quint64>(d));
    }

    m_geoPath.addCoordinate(c);
    emit geoPathChanged();
    //emit geoPathAppend(c);
    if (m_geoPath.size() >= 3) {
        emit geoPathAppend(m_geoPath.path().at(m_geoPath.size() - 3));
    }
}

QGeoRectangle Vehicle::geoPathRect() const
{
    return geoPath().boundingGeoRectangle();
}

void Vehicle::flyHere(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    emit f_mandala->sendValue(mandala::cmd::nav::pos::uid, value);
}
void Vehicle::lookHere(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    emit f_mandala->sendValue(mandala::cmd::nav::gimbal::uid, value);
}
void Vehicle::setHomePoint(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    emit f_mandala->sendValue(mandala::est::nav::ref::uid, value);
}
void Vehicle::sendPositionFix(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    emit f_mandala->sendValue(mandala::est::nav::pos::uid, value);
}

void Vehicle::resetGeoPath()
{
    setGeoPath(QGeoPath());
}

QString Vehicle::fileTitle() const
{
    QString s = confTitle();
    if (s.isEmpty())
        return title();
    return s;
}
QString Vehicle::confTitle() const
{
    //if (protocol()->nodes->size() <= 0)
    //return QString();

    QMap<QString, QString> byName;
    QString shiva;
    QString longest;
    QString anyName;
    foreach (NodeItem *node, f_nodes->nodes()) {
        QString name = node->title();
        QString s = node->value().toString().trimmed();
        if (anyName.isEmpty())
            anyName = name;
        if (s.isEmpty())
            continue;
        int sz = s.size();
        if (byName.value(name).size() < sz)
            byName[name] = s;
        if (name.endsWith(".shiva")) {
            if (shiva.size() < sz)
                shiva = s;
        }
        if (longest.size() < sz)
            longest = s;
    }
    if (!shiva.isEmpty())
        return shiva;
    QString s;
    s = byName.value("nav");
    if (!s.isEmpty())
        return s;
    s = byName.value("com");
    if (!s.isEmpty())
        return s;
    s = byName.value("ifc");
    if (!s.isEmpty())
        return s;
    s = longest;
    if (!s.isEmpty())
        return s;
    return anyName;
}

void Vehicle::message(QString msg, AppNotify::NotifyFlags flags, QString subsystem)
{
    if (subsystem.isEmpty())
        subsystem = title();
    else
        subsystem = QString("%1/%2").arg(title()).arg(subsystem);

    AppNotify::NotifyFlags fType = flags & AppNotify::NotifyTypeMask;

    static const QStringList err(QStringList() << "err"
                                               << "fail"
                                               << "critical"
                                               << "timeout");
    static const QStringList warn(QStringList() << "warn"
                                                << "nofix"
                                                << "missing"
                                                << "none"
                                                << "not found");

    if (fType != AppNotify::Error && fType != AppNotify::Warning) {
        AppNotify::NotifyFlags t = fType;
        for (auto i : err) {
            if (!msg.contains(i, Qt::CaseInsensitive))
                continue;
            t = AppNotify::Error;
            break;
        }
        if (t != AppNotify::Error) {
            for (auto i : warn) {
                if (!msg.contains(i, Qt::CaseInsensitive))
                    continue;
                t = AppNotify::Warning;
                break;
            }
        }
        if (t != fType)
            flags = (flags & ~AppNotify::NotifyTypeMask) | t;
        fType = t;
    }
    AppNotify::instance()->report(msg, flags, subsystem);

    if (fType == AppNotify::Error) {
        f_warnings->error(msg);
    } else if (fType == AppNotify::Warning) {
        f_warnings->warning(msg);
    }
}

QString Vehicle::toolTip() const
{
    QStringList st;
    st.append(Fact::toolTip());
    if (protocol()) {
        auto p = protocol();
        if (!isLocal()) {
            st.append(QString("UID: %1").arg(p->uid()));
            st.append(QString("Type: %1").arg(p->vehicleTypeText()));
        }
        st.append(QString("ErrCnt: %1").arg(p->errcnt()));
    }
    if (!m_info.isEmpty()) {
        st << "";
        st << "[info]";
        st.append(m_info);
    }
    return st.join('\n');
}
bool Vehicle::follow(void) const
{
    return m_follow;
}
void Vehicle::setFollow(const bool &v)
{
    if (m_follow == v)
        return;
    m_follow = v;
    emit followChanged();
}
QGeoCoordinate Vehicle::coordinate(void) const
{
    return m_coordinate;
}
void Vehicle::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
}
Vehicle::FlightState Vehicle::flightState(void) const
{
    return m_flightState;
}
void Vehicle::setFlightState(const FlightState &v)
{
    if (m_flightState == v)
        return;
    m_flightState = v;
    emit flightStateChanged();
}
QGeoPath Vehicle::geoPath(void) const
{
    return m_geoPath;
}
void Vehicle::setGeoPath(const QGeoPath &v)
{
    if (m_geoPath == v)
        return;
    m_geoPath = v;
    emit geoPathChanged();

    //reset total distance
    qreal dist = 0;
    QGeoCoordinate c;
    for (auto p : m_geoPath.path()) {
        if (c.isValid() && p.isValid())
            dist += c.distanceTo(p);
        c = p;
    }
    setTotalDistance(static_cast<quint64>(dist));
}
quint64 Vehicle::totalDistance() const
{
    return m_totalDistance;
}
void Vehicle::setTotalDistance(quint64 v)
{
    if (m_totalDistance == v)
        return;
    m_totalDistance = v;
    emit totalDistanceChanged();
}

PVehicle::StreamType Vehicle::streamType() const
{
    return protocol() ? protocol()->streamType() : PVehicle::OFFLINE;
}
