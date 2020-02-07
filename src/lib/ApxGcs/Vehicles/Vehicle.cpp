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

#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
#include <Telemetry/Telemetry.h>

//=============================================================================
Vehicle::Vehicle(Vehicles *vehicles,
                 QString callsign,
                 quint16 squawk,
                 QString uid,
                 VehicleClass vclass,
                 ProtocolVehicle *protocol)
    : Fact(vclass >= LOCAL ? vehicles : vehicles->f_list, callsign.toLower(), callsign, "", Group)
    , uid(uid)
    , dbKey(0)
    , protocol(protocol)
    , m_streamType(OFFLINE)
    , m_squawk(squawk)
    , m_callsign(callsign)
    , m_vehicleClass(vclass)
    , m_follow(false)
    , m_flightState(FS_UNKNOWN)
    , m_totalDistance(0)
{
    setSection(vehicles->title());
    setIcon(isLocal() ? "chip" : isReplay() ? "play-circle" : "drone");

    connect(this, &Vehicle::callsignChanged, this, &Vehicle::updateTitle);
    connect(this, &Vehicle::streamTypeChanged, this, &Vehicle::updateStatus);

    f_select
        = new Fact(this, "select", tr("Select"), tr("Make this vehicle active"), Action, "select");
    connect(f_select, &Fact::triggered, this, [=]() { vehicles->selectVehicle(this); });
    connect(vehicles, &Vehicles::vehicleSelected, this, [=](Vehicle *v) {
        f_select->setEnabled(v != this);
    });

    f_mandala = new MandalaTree(this);
    f_nodes = new Nodes(this);
    f_mission = new VehicleMission(this);
    f_warnings = new VehicleWarnings(this);
    f_telemetry = new Telemetry(this);

    setMandala(f_mandala);
    if (isLocal()) {
        AppRoot::instance()->setMandala(mandala());
    }

    //Mandala facts binfing
    f_lat = f_mandala->fact(mandala::est::nav::pos::lat::meta.uid);
    f_lon = f_mandala->fact(mandala::est::nav::pos::lon::meta.uid);
    f_hmsl = f_mandala->fact(mandala::est::nav::pos::hmsl::meta.uid);
    f_ref_lat = f_mandala->fact(mandala::est::nav::ref::lat::meta.uid);
    f_ref_lon = f_mandala->fact(mandala::est::nav::ref::lon::meta.uid);
    f_ref_hmsl = f_mandala->fact(mandala::est::nav::ref::hmsl::meta.uid);
    f_vd = f_mandala->fact(mandala::est::nav::pos::vd::meta.uid);
    f_mode = f_mandala->fact(mandala::cmd::nav::op::mode::meta.uid);
    f_stage = f_mandala->fact(mandala::cmd::nav::op::stage::meta.uid);

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

    if (!isTemporary()) {
        connect(vehicles, &Vehicles::vehicleSelected, this, [=](Vehicle *v) {
            setActive(v == this);
            if (active() && (!(isLocal() || isReplay())))
                dlinkReqTimer.start();
            else
                dlinkReqTimer.stop();
        });

        connect(this, &Fact::activeChanged, this, [this]() {
            if (active())
                emit selected();
            setFollow(false);
        });

        onlineTimer.setSingleShot(true);
        onlineTimer.setInterval(7000);
        connect(&onlineTimer, &QTimer::timeout, this, [=]() { setStreamType(OFFLINE); });

        //downlink request timer
        if (!(isLocal() || isReplay())) {
            dlinkReqTimer.setInterval(1000);
            dlinkReqTimer.start();
            connect(&dlinkReqTimer, &QTimer::timeout, this, [=]() {
                if (active())
                    protocol->sendUplink(QByteArray()); //request telemetry
            });
        }
    }

    if (protocol && !isTemporary() && !isReplay()) {
        connect(this,
                &Vehicle::coordinateChanged,
                this,
                &Vehicle::updateGeoPath,
                Qt::QueuedConnection);

        connect(protocol, &ProtocolVehicle::jsexecData, this, &Vehicle::jsexecData);
    }
    if (!isTemporary()) {
        Fact *f = new Fact(f_telemetry,
                           "rpath",
                           tr("Reset Path"),
                           tr("Clear travelled path"),
                           Action,
                           "history");
        connect(f, &Fact::triggered, this, &Vehicle::resetGeoPath);
        connect(this, &Vehicle::geoPathChanged, f, [this, f]() {
            f->setEnabled(!geoPath().isEmpty());
        });
        f->setEnabled(false);
    }

    updateStatus();
    updateInfo();

    //connect protocols
    if (protocol) {
        connect(protocol, &QObject::destroyed, this, [this]() { this->protocol = nullptr; });
        //status and stream type
        connect(protocol, &ProtocolVehicle::xpdrData, this, &Vehicle::setStreamXpdr);
        connect(protocol->telemetry,
                &ProtocolTelemetry::telemetryDataReceived,
                this,
                &Vehicle::setStreamTelemetry);
        connect(protocol->telemetry,
                &ProtocolTelemetry::valueDataReceived,
                this,
                &Vehicle::setStreamData);
        connect(protocol, &ProtocolVehicle::serialRxData, this, &Vehicle::setStreamData);
        connect(protocol, &ProtocolVehicle::serialTxData, this, &Vehicle::setStreamData);

        connect(protocol->telemetry,
                &ProtocolTelemetry::mandalaValueReceived,
                this,
                &Vehicle::updateDatalinkVars);

        //recorder
        connect(protocol, &ProtocolVehicle::xpdrData, this, &Vehicle::recordDownlink);
        connect(protocol->telemetry,
                &ProtocolTelemetry::telemetryDataReceived,
                this,
                &Vehicle::recordDownlink);
        connect(protocol->telemetry,
                &ProtocolTelemetry::valueDataReceived,
                this,
                &Vehicle::recordDownlink);
        connect(protocol, &ProtocolVehicle::serialRxData, this, [this](uint portNo, QByteArray data) {
            emit recordSerialData(static_cast<quint8>(portNo), data, false);
        });
        connect(protocol, &ProtocolVehicle::serialTxData, this, [this](uint portNo, QByteArray data) {
            emit recordSerialData(static_cast<quint8>(portNo), data, true);
        });
        //connect(this,&Vehicle::recordNodes,this,&Vehicle::dbSaveConfigDataBackup);

        //ident update
        connect(protocol, &ProtocolVehicle::identUpdated, this, [this, protocol]() {
            setSquawk(protocol->squawk);
            setCallsign(protocol->ident.callsign);
            setVehicleClass(static_cast<Vehicle::VehicleClass>(protocol->ident.vclass));
            apxMsg() << tr("Vehicle IDENT updated").append(":") << vehicleClassText()
                     << "'" + this->callsign() + "'"
                     << "(" + squawkText() + ")";
        });

        //forward signals
        connect(protocol->telemetry,
                &ProtocolTelemetry::telemetryDataReceived,
                this,
                &Vehicle::telemetryDataReceived);
        connect(protocol->telemetry,
                &ProtocolTelemetry::valueDataReceived,
                this,
                &Vehicle::valueDataReceived);
        connect(protocol, &ProtocolVehicle::serialRxData, this, &Vehicle::serialRxDataReceived);
        connect(protocol, &ProtocolVehicle::serialTxData, this, &Vehicle::serialTxDataReceived);
    }

    //register JS new vehicles instantly
    connect(this, &Vehicle::nameChanged, this, [=]() { App::jsync(this); });
    App::jsync(this);
}
Vehicle::~Vehicle()
{
    qDebug() << "vehicle removed";
}
//=============================================================================
//=============================================================================
void Vehicle::dbSaveVehicleInfo()
{
    if (isReplay())
        return;
    QVariantMap info;
    info.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    info.insert("uid", uid);
    info.insert("callsign", callsign());
    info.insert("class", vehicleClassText());
    info.insert("squawk", squawkText());
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
//=============================================================================
//=============================================================================
void Vehicle::updateTitle()
{
    setName(callsign());
    setTitle(callsign());
}
void Vehicle::updateStatus()
{
    setValue(streamTypeText());
    f_mandala->setValue(value());
}
void Vehicle::updateInfo()
{
    QStringList st;
    //st<<callsign();
    if (vehicleClass() != GCU) {
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
//=============================================================================
bool Vehicle::isLocal() const
{
    return vehicleClass() == LOCAL;
}
bool Vehicle::isReplay() const
{
    return !protocol; // vehicleClass()==REPLAY;
}
bool Vehicle::isTemporary() const
{
    return vehicleClass() == TEMPORARY;
}
void Vehicle::setReplay(bool v)
{
    if (v) {
        setStreamType(TELEMETRY);
        onlineTimer.start();
    } else if (isReplay()) {
        onlineTimer.stop();
        setStreamType(OFFLINE);
    }
}
QGeoRectangle Vehicle::geoPathRect() const
{
    return geoPath().boundingGeoRectangle();
}
//=============================================================================
QString Vehicle::streamTypeText() const
{
    return QMetaEnum::fromType<StreamType>().valueToKey(streamType());
}
QString Vehicle::vehicleClassText() const
{
    return QMetaEnum::fromType<VehicleClass>().valueToKey(vehicleClass());
}
QString Vehicle::squawkText() const
{
    return squawkText(squawk());
}
QString Vehicle::squawkText(quint16 v) const
{
    return QString::number(v, 16).toUpper();
}
//=============================================================================
//=============================================================================
void Vehicle::setStreamXpdr()
{
    //emit recordDownlink(data);
    setStreamType(XPDR);
    xpdrTime.start();
    onlineTimer.start();
}
void Vehicle::setStreamTelemetry()
{
    //emit recordDownlink(packet);
    setStreamType(TELEMETRY);
    telemetryTime.start();
    onlineTimer.start();
}
void Vehicle::setStreamData()
{
    if (telemetryTime.elapsed() > 2000 && xpdrTime.elapsed() > 3000) {
        setStreamType(DATA);
    }
    onlineTimer.start();
}
void Vehicle::setStreamService()
{
    //emit nmtReceived(packet);
    if (telemetryTime.elapsed() > 2000 && xpdrTime.elapsed() > 3000)
        setStreamType(SERVICE);
    onlineTimer.start();
}
//=============================================================================
void Vehicle::jsexecData(QByteArray data)
{
    if (data.size() < 3) {
        qDebug() << "Empty jsexec data received";
        return;
    }
    App::instance()->engine()->jsexec(data);
}
//=============================================================================
//=============================================================================
void Vehicle::vmexec(QString func)
{
    if (protocol)
        protocol->vmexec(func);
}
void Vehicle::sendSerial(quint8 portID, QByteArray data)
{
    if (protocol)
        protocol->sendSerial(portID, data);
}
//=============================================================================
void Vehicle::flyHere(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    const QGeoCoordinate h(f_ref_lat->value().toDouble(), f_ref_lon->value().toDouble());
    qreal azimuth_r = qDegreesToRadians(h.azimuthTo(c));
    qreal distance = h.distanceTo(c);
    qreal n = std::cos(azimuth_r) * distance;
    qreal e = std::sin(azimuth_r) * distance;
    protocol->telemetry->sendPointValue(mandala::cmd::nav::pos::n::meta.uid, n, e);
}
void Vehicle::lookHere(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    double hmsl = f_ref_hmsl->value().toDouble();
    protocol->telemetry->sendVectorValue(mandala::cmd::nav::gimbal::lat::meta.uid,
                                         c.latitude(),
                                         c.longitude(),
                                         hmsl);
}
void Vehicle::setHomePoint(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    double hmsl = f_ref_hmsl->value().toDouble();
    protocol->telemetry->sendVectorValue(mandala::est::nav::ref::lat::meta.uid,
                                         c.latitude(),
                                         c.longitude(),
                                         hmsl);
}
void Vehicle::sendPositionFix(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    double hmsl = f_hmsl->value().toDouble();
    protocol->telemetry->sendVectorValue(mandala::sns::nav::gps::lat::meta.uid,
                                         c.latitude(),
                                         c.longitude(),
                                         hmsl);
}
//=============================================================================
void Vehicle::resetGeoPath()
{
    setGeoPath(QGeoPath());
}
//=============================================================================
//=============================================================================
QString Vehicle::fileTitle() const
{
    QString s = confTitle();
    if (s.isEmpty())
        return title();
    return s;
}
QString Vehicle::confTitle() const
{
    if (!(f_nodes->nodesCount() > 0 && f_nodes->dictValid() && f_nodes->dataValid())) {
        return QString();
    }
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
//=============================================================================
void Vehicle::message(QString msg, AppNotify::NotifyFlags flags, QString subsystem)
{
    if (isTemporary())
        return;

    if (subsystem.isEmpty())
        subsystem = callsign();
    else
        subsystem = QString("%1/%2").arg(callsign()).arg(subsystem);

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
//=============================================================================
//=============================================================================
void Vehicle::updateDatalinkVars(quint16 id, double)
{
    /*switch (id) {
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
//=============================================================================
//=============================================================================
Vehicle::StreamType Vehicle::streamType(void) const
{
    return m_streamType;
}
void Vehicle::setStreamType(const StreamType v)
{
    if (m_streamType == v)
        return;
    m_streamType = v;
    emit streamTypeChanged();
}
quint16 Vehicle::squawk(void) const
{
    return m_squawk;
}
void Vehicle::setSquawk(const quint16 v)
{
    if (m_squawk == v)
        return;
    m_squawk = v;
    emit squawkChanged();
}
QString Vehicle::callsign(void) const
{
    return m_callsign;
}
void Vehicle::setCallsign(const QString &v)
{
    if (m_callsign == v)
        return;
    m_callsign = v;
    emit callsignChanged();
}
Vehicle::VehicleClass Vehicle::vehicleClass(void) const
{
    return m_vehicleClass;
}
void Vehicle::setVehicleClass(const VehicleClass v)
{
    if (m_vehicleClass == v)
        return;
    m_vehicleClass = v;
    emit vehicleClassChanged();
}
void Vehicle::setVehicleClass(const QString &v)
{
    bool ok = false;
    int i = QMetaEnum::fromType<VehicleClass>().keyToValue(v.toUtf8(), &ok);
    if (!ok)
        return;
    setVehicleClass(static_cast<VehicleClass>(i));
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
uint Vehicle::errcnt(void) const
{
    return m_errcnt;
}
bool Vehicle::setErrcnt(const uint &v)
{
    if (m_errcnt == v)
        return false;
    m_errcnt = v;
    emit errcntChanged();
    return true;
}
//=============================================================================
