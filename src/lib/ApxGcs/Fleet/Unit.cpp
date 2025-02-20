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
#include "Unit.h"
#include "Fleet.h"
#include "UnitWarnings.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <ApxMisc/JsonHelpers.h>
#include <Mandala/Mandala.h>
#include <Mission/UnitMission.h>
#include <Nodes/Nodes.h>
#include <Telemetry/Telemetry.h>

Unit::Unit(Fleet *fleet, PUnit *protocol)
    : Fact(fleet, protocol ? protocol->name() : "replay", protocol ? protocol->title() : "REPLAY")
    , _protocol(protocol)
{
    if (protocol) {
        bindProperty(protocol, "title", true);
        bindProperty(protocol, "value", true);
        bindProperty(protocol, "active");

        // unit deletion
        connect(this, &Fact::removed, protocol, &Fact::deleteFact);

        connect(protocol, &PUnit::streamTypeChanged, this, &Unit::streamTypeChanged);

        if (protocol->uid().isEmpty()) {
            m_is_local = true;
            setName("local");
        } else {
            m_is_identified = true;
            m_is_gcs = protocol->unitType() == PUnit::GCS;
            connect(protocol, &PUnit::unitTypeChanged, this, [this]() {
                m_is_gcs = _protocol->unitType() == PUnit::GCS;
                emit isGroundControlChanged();
            });
        }
    } else {
        m_is_replay = true;
    }

    setIcon(m_is_identified ? "drone" : "chip");

    f_select = new Fact(this, "select", tr("Select"), tr("Make this unit active"), Action, "select");
    connect(f_select, &Fact::triggered, this, [this, fleet]() { fleet->selectUnit(this); });

    f_mandala = new Mandala(this);
    f_nodes = new Nodes(this);
    f_mission = new UnitMission(this);
    f_warnings = new UnitWarnings(this);

    if (isIdentified()) {
        auto f_remove = new Fact(this,
                                 "remove",
                                 tr("Remove"),
                                 tr("Remove unit"),
                                 Action | Remove | IconOnly);
        connect(f_remove, &Fact::triggered, this, &Unit::deleteUnit);
        connect(this, &Unit::deleteUnit, fleet, [this, fleet]() { fleet->deleteUnit(this); });
    }

    // f_lookup = new LookupUnitConfig(this, this);
    f_storage = new UnitStorage(this);
    f_share = new UnitShare(this, this);

    setMandala(f_mandala);

    //Mandala facts binfing
    f_lat = f_mandala->fact("est.pos.lat");
    f_lon = f_mandala->fact("est.pos.lon");
    f_hmsl = f_mandala->fact("est.pos.hmsl");

    f_ref_hmsl = f_mandala->fact("est.ref.hmsl");

    f_vspeed = f_mandala->fact("est.pos.vspeed");
    f_mode = f_mandala->fact("cmd.proc.mode");
    f_stage = f_mandala->fact("cmd.proc.stage");

    updateInfoTimer.setInterval(300);
    updateInfoTimer.setSingleShot(true);
    connect(&updateInfoTimer, &QTimer::timeout, this, &Unit::updateInfo);

    connect(f_lat, &Fact::valueChanged, this, &Unit::updateCoordinate);
    connect(f_lon, &Fact::valueChanged, this, &Unit::updateCoordinate);
    connect(f_hmsl, &Fact::valueChanged, this, &Unit::updateInfoReq);
    connect(f_vspeed, &Fact::valueChanged, this, &Unit::updateInfoReq);
    connect(f_mode, &Fact::valueChanged, this, &Unit::updateInfoReq);
    connect(f_stage, &Fact::valueChanged, this, &Unit::updateInfoReq);

    connect(f_mode, &Fact::valueChanged, this, &Unit::updateFlightState);
    connect(f_stage, &Fact::valueChanged, this, &Unit::updateFlightState);

    connect(this, &Fact::activeChanged, this, &Unit::updateActive);
    connect(f_nodes, &Nodes::upgradingChanged, this, &Unit::updateActive);

    if (protocol) {
        connect(this, &Unit::coordinateChanged, this, &Unit::updateGeoPath, Qt::QueuedConnection);

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
        connect(this, &Unit::titleChanged, f_storage, &UnitStorage::saveUnitInfo);
        connect(this, &Unit::isGroundControlChanged, f_storage, &UnitStorage::saveUnitInfo);

        // counters
        connect(protocol, &PUnit::packetReceived, this, &Unit::packetReceived);

        // forward serial TX for plugins
        connect(this, &Unit::requestScript, protocol->data(), &PData::requestScript);
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
    // will connect to protocol by itself and must be created after unit's protocol connections
    f_telemetry = new Telemetry(this);

    // path
    auto f_rpath = new Fact(f_telemetry,
                            "rpath",
                            tr("Reset Path"),
                            tr("Clear travelled path"),
                            Action,
                            "history");
    f_rpath->move(0);
    connect(f_rpath, &Fact::triggered, this, &Unit::resetGeoPath);
    connect(this, &Unit::geoPathChanged, f_rpath, [this, f_rpath]() {
        f_rpath->setEnabled(!geoPath().isEmpty());
    });
    f_rpath->setEnabled(false);

    setOpt("VID", uid());

    updateInfo();

    if (isIdentified()) {
        f_storage->saveUnitInfo();
    }

    App::jsync(this);
}

void Unit::packetReceived(mandala::uid_t uid)
{
    if (xbus::cmd::match(uid)) {
        MandalaFact *f = f_mandala->fact(uid);
        if (f)
            f->increment_rx_cnt();
    }
}

void Unit::updateActive()
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

QJsonObject Unit::get_info() const
{
    if (!_protocol)
        return _importedUnitInfo;

    QJsonObject info;
    info.insert("uid", _protocol->uid());
    info.insert("name", title());
    info.insert("type", _protocol->unitTypeText());

    if (_lastSeenTime)
        info.insert("time", _lastSeenTime);

    qDebug() << _lastSeenTime << title() << m_is_identified;
    return info;
}
QJsonValue Unit::toJson()
{
    QJsonObject jso;

    jso.insert("unit", get_info());
    jso.insert("nodes", f_nodes->toJson());
    jso.insert("title", f_nodes->getConfigTitle());
    jso.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());

    return jso;
}
void Unit::fromJson(const QJsonValue &jsv)
{
    // json::save("unit-fromJson-" + title(), jsv);

    const auto jso = jsv.toObject();

    if (!_protocol)
        _importedUnitInfo = jso.value("unit").toObject();

    const auto nodes = jso.value("nodes").toArray();
    if (nodes.isEmpty()) {
        apxMsgW() << tr("Missing nodes in data set");
    } else {
        f_nodes->fromJson(nodes);
    }
}

void Unit::updateInfo()
{
    if (isIdentified()) {
        _lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }
    QStringList st;
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
void Unit::updateInfoReq()
{
    if (updateInfoTimer.isActive())
        return;
    updateInfoTimer.start();
}
void Unit::updateCoordinate()
{
    setCoordinate(QGeoCoordinate(f_lat->value().toDouble(),
                                 f_lon->value().toDouble(),
                                 f_hmsl->value().toDouble()));
}
void Unit::updateFlightState()
{
    if ((f_mode->value().toUInt() == mandala::proc_mode_LANDING)
        && (f_stage->value().toUInt() >= 7)) {
        //setFlightState(FS_LANDED);
        // TODO improve landed condition detector, independently checking the state of the unit
    } else if ((f_mode->value().toUInt() == mandala::proc_mode_TAKEOFF)
               && (f_stage->value().toUInt() >= 2) && (f_stage->value().toUInt() < 100)) {
        setFlightState(FS_TAKEOFF);
    } else
        setFlightState(FS_UNKNOWN);
}
void Unit::updateGeoPath()
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

QGeoRectangle Unit::geoPathRect() const
{
    return geoPath().boundingGeoRectangle();
}

void Unit::flyHere(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    f_mandala->fact("cmd.pos")->sendValue(value);
}
void Unit::lookHere(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    f_mandala->fact("cmd.gimbal")->sendValue(value);
}
void Unit::setHomePoint(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    f_mandala->fact("est.ref")->sendValue(value);
}
void Unit::sendPositionFix(QGeoCoordinate c)
{
    if (isReplay())
        return;
    if (!c.isValid())
        return;
    QVariantList value;
    value << c.latitude();
    value << c.longitude();
    f_mandala->fact("est.pos")->sendValue(value);
}

void Unit::resetGeoPath()
{
    setGeoPath(QGeoPath());
    setTotalDistance(0);
}

QString Unit::fileTitle() const
{
    QString s = confTitle();
    if (s.isEmpty())
        return title();
    return s;
}
QString Unit::confTitle() const
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

void Unit::message(QString msg, AppNotify::NotifyFlags flags, QString subsystem, QString src_uid)
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

    emit messageReported(msg, subsystem, src_uid);

    if (fType == AppNotify::Error) {
        f_warnings->error(msg);
    } else if (fType == AppNotify::Warning) {
        f_warnings->warning(msg);
    }
}

QString Unit::toolTip() const
{
    QStringList st;
    st.append(Fact::toolTip());
    if (protocol()) {
        auto p = protocol();
        if (!isLocal()) {
            st.append(QString("UID: %1").arg(p->uid()));
            st.append(QString("Type: %1").arg(p->unitTypeText()));
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
bool Unit::follow(void) const
{
    return m_follow;
}
void Unit::setFollow(const bool &v)
{
    if (m_follow == v)
        return;
    m_follow = v;
    emit followChanged();
}
QGeoCoordinate Unit::coordinate(void) const
{
    return m_coordinate;
}
void Unit::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
}
Unit::FlightState Unit::flightState(void) const
{
    return m_flightState;
}
void Unit::setFlightState(const FlightState &v)
{
    if (m_flightState == v)
        return;
    m_flightState = v;
    emit flightStateChanged();
}
QGeoPath Unit::geoPath(void) const
{
    return m_geoPath;
}
void Unit::setGeoPath(const QGeoPath &v)
{
    if (m_geoPath == v)
        return;
    m_geoPath = v;
    emit geoPathChanged();
}
quint64 Unit::totalDistance() const
{
    return m_totalDistance;
}
void Unit::setTotalDistance(quint64 v)
{
    if (m_totalDistance == v)
        return;
    m_totalDistance = v;
    emit totalDistanceChanged();
}

PUnit::StreamType Unit::streamType() const
{
    return protocol() ? protocol()->streamType() : PUnit::OFFLINE;
}
