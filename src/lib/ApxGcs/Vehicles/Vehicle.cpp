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
#include "Vehicle.h"
#include "VehicleWarnings.h"
#include "Vehicles.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <Database/NodesReqVehicle.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
#include <Telemetry/Telemetry.h>

Vehicle::Vehicle(Vehicles *vehicles, ProtocolVehicle *protocol)
    : ProtocolViewBase(vehicles, protocol)
{
    setSection(vehicles->title());

    f_select
        = new Fact(this, "select", tr("Select"), tr("Make this vehicle active"), Action, "select");
    connect(f_select, &Fact::triggered, this, [this, vehicles]() { vehicles->selectVehicle(this); });

    connect(this, &Fact::activeChanged, this, &Vehicle::updateActive);

    f_mandala = new Mandala(this);
    f_nodes = new Nodes(this, protocol->nodes);
    f_mission = new VehicleMission(this);
    f_warnings = new VehicleWarnings(this);
    f_telemetry = new Telemetry(this);

    setMandala(f_mandala);
    if (isLocal()) {
        AppRoot::instance()->setMandala(mandala());
    }

    //Mandala facts binfing
    f_lat = f_mandala->fact(mandala::est::nav::pos::lat::uid);
    f_lon = f_mandala->fact(mandala::est::nav::pos::lon::uid);
    f_hmsl = f_mandala->fact(mandala::est::nav::pos::hmsl::uid);
    f_ref_lat = f_mandala->fact(mandala::est::nav::ref::lat::uid);
    f_ref_lon = f_mandala->fact(mandala::est::nav::ref::lon::uid);
    f_ref_hmsl = f_mandala->fact(mandala::est::nav::ref::hmsl::uid);
    f_vd = f_mandala->fact(mandala::est::nav::pos::vd::uid);
    f_mode = f_mandala->fact(mandala::cmd::nav::op::mode::uid);
    f_stage = f_mandala->fact(mandala::cmd::nav::op::stage::uid);
    f_cmd_n = f_mandala->fact(mandala::cmd::nav::pos::n::uid);
    f_cmd_e = f_mandala->fact(mandala::cmd::nav::pos::e::uid);

    f_cmd_gimbal_lat = f_mandala->fact(mandala::cmd::nav::gimbal::lat::uid);
    f_cmd_gimbal_lon = f_mandala->fact(mandala::cmd::nav::gimbal::lon::uid);
    f_cmd_gimbal_hmsl = f_mandala->fact(mandala::cmd::nav::gimbal::hmsl::uid);

    f_gps_lat = f_mandala->fact(mandala::sns::nav::gps::lat::uid);
    f_gps_lon = f_mandala->fact(mandala::sns::nav::gps::lon::uid);
    f_gps_hmsl = f_mandala->fact(mandala::sns::nav::gps::hmsl::uid);

    updateInfoTimer.setInterval(300);
    updateInfoTimer.setSingleShot(true);
    connect(&updateInfoTimer, &QTimer::timeout, this, &Vehicle::updateInfo);

    connect(f_lat, &Fact::valueChanged, this, &Vehicle::updateCoordinate);
    connect(f_lon, &Fact::valueChanged, this, &Vehicle::updateCoordinate);
    connect(f_hmsl, &Fact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_vd, &Fact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_mode, &Fact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_stage, &Fact::valueChanged, this, &Vehicle::updateInfoReq);

    connect(f_mode, &Fact::valueChanged, this, &Vehicle::updateFlightState);
    connect(f_stage, &Fact::valueChanged, this, &Vehicle::updateFlightState);

    //downlink request timer
    if (!(isLocal() || isReplay())) {
        telemetryReqTimer.setInterval(1000);
        connect(&telemetryReqTimer, &QTimer::timeout, protocol, &ProtocolVehicle::requestTelemetry);
    }

    if (!isReplay()) {
        connect(this,
                &Vehicle::coordinateChanged,
                this,
                &Vehicle::updateGeoPath,
                Qt::QueuedConnection);

        //mandala update signals
        connect(f_mandala, &Mandala::sendUplink, protocol, &ProtocolVehicle::send);

        connect(protocol, &ProtocolVehicle::identChanged, this, &Vehicle::identChanged);
        connect(protocol, &ProtocolVehicle::jsexecData, this, &Vehicle::jsexecData);

        //FIXME: connect(protocol, &ProtocolVehicle::receivedData, this, &Vehicle::updateDatalinkVars);

        //recorder
        connect(protocol, &ProtocolVehicle::xpdrData, this, &Vehicle::recordDownlink);
        connect(protocol, &ProtocolVehicle::telemetryData, this, &Vehicle::recordDownlink);
        connect(protocol, &ProtocolVehicle::receivedData, this, &Vehicle::recordDownlink);
        connect(protocol, &ProtocolVehicle::serialRxData, this, [this](uint portNo, QByteArray data) {
            emit recordSerialData(static_cast<quint8>(portNo), data, false);
        });
        connect(protocol, &ProtocolVehicle::serialTxData, this, [this](uint portNo, QByteArray data) {
            emit recordSerialData(static_cast<quint8>(portNo), data, true);
        });
    }

    Fact *f = new Fact(f_telemetry,
                       "rpath",
                       tr("Reset Path"),
                       tr("Clear travelled path"),
                       Action,
                       "history");
    connect(f, &Fact::triggered, this, &Vehicle::resetGeoPath);
    connect(this, &Vehicle::geoPathChanged, f, [this, f]() { f->setEnabled(!geoPath().isEmpty()); });
    f->setEnabled(false);

    updateInfo();

    //register JS new vehicles instantly
    connect(this, &Vehicle::nameChanged, this, [=]() { App::jsync(this); });
    App::jsync(this);
}
Vehicle::~Vehicle()
{
    qDebug() << "vehicle removed";
}

void Vehicle::updateActive()
{
    bool v = active();
    f_select->setEnabled(!v);

    if (v)
        telemetryReqTimer.start();
    else
        telemetryReqTimer.stop();

    if (active())
        emit selected();

    setFollow(false);
}

void Vehicle::dbSaveVehicleInfo()
{
    if (isReplay())
        return;
    QVariantMap info;
    info.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    info.insert("uid", uid);
    info.insert("callsign", title());
    info.insert("squawk", protocol()->squawkText());
    DBReqSaveVehicleInfo *req = new DBReqSaveVehicleInfo(info);
    connect(req,
            &DBReqSaveVehicleInfo::foundID,
            this,
            &Vehicle::dbSetVehicleKey,
            Qt::QueuedConnection);
    req->exec();
}
void Vehicle::dbSetVehicleKey(quint64 key)
{
    dbKey = key;
}

void Vehicle::updateInfo()
{
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

        int vs = -f_vd->value().toInt();
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
    if ((f_mode->value().toUInt() == mandala::op_mode_LANDING)
        && (f_stage->value().toUInt() >= 250)) {
        setFlightState(FS_LANDED);
    } else if ((f_mode->value().toUInt() == mandala::op_mode_TAKEOFF)
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

bool Vehicle::isLocal() const
{
    return protocol()->enabled() && protocol()->squawk() == 0
           && protocol()->ident().flags.bits.gcs == 0;
}
bool Vehicle::isReplay() const
{
    return protocol()->enabled() == false;
}
bool Vehicle::isGroundControl() const
{
    return protocol()->enabled() && protocol()->squawk() && protocol()->ident().flags.bits.gcs;
}

QGeoRectangle Vehicle::geoPathRect() const
{
    return geoPath().boundingGeoRectangle();
}

void Vehicle::jsexecData(QString data)
{
    App::instance()->engine()->jsexec(data);
}
void Vehicle::vmexec(QString func)
{
    protocol()->vmexec(func);
}
void Vehicle::sendSerial(quint8 portID, QByteArray data)
{
    protocol()->sendSerial(portID, data);
}

void Vehicle::flyHere(const QGeoCoordinate &c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    const QGeoCoordinate h(f_ref_lat->value().toDouble(), f_ref_lon->value().toDouble());
    qreal azimuth_r = qDegreesToRadians(h.azimuthTo(c));
    qreal distance = h.distanceTo(c);
    QVariantList vlist;
    vlist << std::cos(azimuth_r) * distance;
    vlist << std::sin(azimuth_r) * distance;
    f_cmd_n->setValues(vlist);
}
void Vehicle::lookHere(const QGeoCoordinate &c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList vlist;
    vlist << c.latitude();
    vlist << c.longitude();
    vlist << f_ref_hmsl->value();
    f_cmd_gimbal_lat->setValues(vlist);
}
void Vehicle::setHomePoint(const QGeoCoordinate &c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList vlist;
    vlist << c.latitude();
    vlist << c.longitude();
    vlist << f_ref_hmsl->value();
    f_ref_lat->setValues(vlist);
}
void Vehicle::sendPositionFix(const QGeoCoordinate &c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList vlist;
    vlist << c.latitude();
    vlist << c.longitude();
    vlist << f_hmsl->value();
    f_gps_lat->setValues(vlist);
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
    if (protocol()->nodes->size() <= 0)
        return QString();

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
    s = byName.value("mhx");
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
    // FIXME: if (isTemporary()) return;

    if (subsystem.isEmpty())
        subsystem = title();
    else
        subsystem = QString("%1/%2").arg(title()).arg(subsystem);

    AppNotify::NotifyFlags fType = flags & AppNotify::NotifyTypeMask;

    if (fType != AppNotify::Error && fType != AppNotify::Warning) {
        AppNotify::NotifyFlags t = fType;
        if (msg.contains("err", Qt::CaseInsensitive))
            t = AppNotify::Error;
        else if (msg.contains("fail", Qt::CaseInsensitive))
            t = AppNotify::Error;
        else if (msg.contains("timeout", Qt::CaseInsensitive))
            t = AppNotify::Error;
        else if (msg.contains("warn", Qt::CaseInsensitive))
            t = AppNotify::Warning;
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

void Vehicle::updateDatalinkVars(quint16 id, QByteArray)
{
    /*FIXME:
    switch (id) {
    default:
        return;
    case mandala::idx_gcu_RSS:
    case mandala::idx_gcu_Ve:
    case mandala::idx_gcu_MT:
        break;
    }
    Fact *fdest = Vehicles::instance()->f_local->f_mandala->factById(id);
    if (!fdest)
        return;
    Fact *fsrc = f_mandala->factById(id);
    if (!fsrc)
        return;
    fdest->setValue(fsrc->value());*/
}

QString Vehicle::info(void) const
{
    return m_info;
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
