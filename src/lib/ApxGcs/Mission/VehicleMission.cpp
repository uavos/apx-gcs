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
#include "VehicleMission.h"
#include "LookupMissions.h"
#include "MissionField.h"
#include "MissionGroup.h"
#include "MissionListModel.h"
#include "MissionShare.h"
#include "MissionStorage.h"
#include "MissionTools.h"

#include "Area.h"
#include "Poi.h"
#include "Runway.h"
#include "Taxiway.h"
#include "Waypoint.h"

#include <App/App.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>
#include <QQmlEngine>

#define MISSION_FORMAT "11"

VehicleMission::VehicleMission(Vehicle *parent)
    : Fact(parent, "mission", "Mission", tr("Vehicle mission"), Group | ModifiedGroup, "ship-wheel")
    , vehicle(parent)
    , blockSizeUpdate(false)
    , m_startHeading(0)
    , m_startLength(0)
    , m_missionSize(0)
    , m_empty(true)
    , m_synced(false)
    , m_saved(false)
    , m_selectedItem(nullptr)
{
    setOpt("pos", QPointF(0, 1));

    storage = new MissionStorage(this);
    connect(storage, &MissionStorage::loaded, this, [this]() {
        if (!empty())
            emit missionAvailable();
    });

    f_title = new MissionField(this, "mission_title", tr("Title"), tr("Mission title"), Text);
    f_title->setIcon("square-edit-outline");

    connect(f_title, &Fact::textChanged, this, &VehicleMission::updateStatus);
    connect(this, &VehicleMission::missionSizeChanged, this, &VehicleMission::updateStatus);

    //groups of items
    f_runways = new Runways(this,
                            "rw",
                            tr("Runways"),
                            tr("Takeoff and Landing"),
                            vehicle->f_mandala->fact(mandala::cmd::nav::proc::rw::uid));
    f_waypoints = new Waypoints(this,
                                "wp",
                                tr("Waypoints"),
                                "",
                                vehicle->f_mandala->fact(mandala::cmd::nav::proc::wp::uid));
    f_pois = new Pois(this,
                      "pi",
                      tr("Points"),
                      tr("Points of Interest"),
                      vehicle->f_mandala->fact(mandala::cmd::nav::proc::pi::uid));
    f_taxiways = new Taxiways(this,
                              "tw",
                              tr("Taxiways"),
                              "",
                              vehicle->f_mandala->fact(mandala::cmd::nav::proc::wp::uid));
    f_areas = new Areas(this, "area", tr("Area"), tr("Airspace definitions"));

    foreach (MissionGroup *group, groups) {
        connect(group, &Fact::sizeChanged, this, &VehicleMission::updateSize, Qt::QueuedConnection);
    }

    //actions
    f_request
        = new Fact(this, "request", tr("Request"), tr("Download from vehicle"), Action, "download");
    connect(f_request, &Fact::triggered, this, &VehicleMission::downloadMission);

    f_upload
        = new Fact(this, "upload", tr("Upload"), tr("Upload to vehicle"), Action | Apply, "upload");
    connect(f_upload, &Fact::triggered, this, &VehicleMission::uploadMission);

    f_clear = new Fact(this, "clear", tr("Clear"), tr("Clear mission"), Action | Remove | IconOnly);
    connect(f_clear, &Fact::triggered, this, &VehicleMission::clearMission);

    //tools actions
    f_tools = new MissionTools(this, Action | IconOnly);

    f_lookup = new LookupMissions(this, f_tools, Action);

    f_save = new Fact(f_tools,
                      "save",
                      tr("Save"),
                      tr("Save mission to database"),
                      Action | Apply | IconOnly,
                      "content-save");
    connect(f_save, &Fact::triggered, storage, &MissionStorage::saveMission);

    f_share = new MissionShare(this, f_tools, Action | IconOnly);
    //connect(f_share->f_export, &Fact::triggered, f_save, &Fact::trigger);

    //App::jsync(f_tools);

    foreach (FactBase *a, actions()) {
        connect(static_cast<Fact *>(a),
                &Fact::enabledChanged,
                this,
                &VehicleMission::actionsUpdated);
    }

    //internal
    m_listModel = new MissionListModel(this);

    connect(this, &VehicleMission::emptyChanged, this, &VehicleMission::updateActions);
    connect(this, &VehicleMission::savedChanged, this, &VehicleMission::updateActions);
    connect(this, &VehicleMission::syncedChanged, this, &VehicleMission::updateActions);

    connect(this, &VehicleMission::startPointChanged, this, &VehicleMission::updateStartPath);
    connect(this, &VehicleMission::startHeadingChanged, this, &VehicleMission::updateStartPath);
    connect(this, &VehicleMission::startLengthChanged, this, &VehicleMission::updateStartPath);

    //sync and saved status behavior
    connect(this, &Fact::modifiedChanged, this, [this]() {
        if (modified()) {
            setSynced(false);
            setSaved(false);
        }
    });
    connect(this, &VehicleMission::emptyChanged, this, [this]() {
        if (empty()) {
            setSynced(false);
            setSaved(true);
        }
    });
    connect(this, &VehicleMission::missionAvailable, this, [=]() {
        setSynced(true);
        setSaved(false);
    });
    connect(storage, &MissionStorage::loaded, this, [this]() {
        setSynced(false);
        setSaved(true);
    });
    connect(storage, &MissionStorage::saved, this, [this]() {
        setSaved(true);
        backup();
    });

    //protocols
    if (vehicle->protocol()) {
        // connect(vehicle->protocol()->mission,
        //         &ProtocolMission::uploaded,
        //         this,
        //         &VehicleMission::missionUploaded);

        connect(vehicle->protocol()->mission(),
                &PMission::missionReceived,
                this,
                &VehicleMission::missionReceived);

        // connect(vehicle->protocol()->mission(), &PMission::missionAvailable, this, [this]() {
        //     if (empty())
        //         vehicle->protocol()->mission()->requestMission();
        // });

        // FIXME: mission protocol
        /*  connect(vehicle->protocol->mission,
                &ProtocolMission::missionDataError,
                this,
                &VehicleMission::missionDataError);

        if (!vehicle->isLocal()) {
            QTimer::singleShot(2000, vehicle->protocol->mission, &ProtocolMission::downloadMission);
        }*/
    }

    //reset and update
    clearMission();
    updateActions();
    updateStatus();

    qmlRegisterUncreatableType<VehicleMission>("APX.Mission", 1, 0, "Mission", "Reference only");
    qmlRegisterUncreatableType<MissionItem>("APX.Mission", 1, 0, "MissionItem", "Reference only");
    qmlRegisterUncreatableType<Waypoint>("APX.Mission", 1, 0, "Waypoint", "Reference only");
    qmlRegisterUncreatableType<Runway>("APX.Mission", 1, 0, "Runway", "Reference only");
    qmlRegisterUncreatableType<MissionListModel>("APX.Mission",
                                                 1,
                                                 0,
                                                 "MissionListModel",
                                                 "Reference only");

    connect(this, &Fact::triggered, this, [this]() {
        if (missionSize() <= 0)
            f_request->trigger();
    });

    App::jsync(this);
}

void VehicleMission::updateActions()
{
    bool bEmpty = empty();
    f_upload->setEnabled(!bEmpty);
    f_clear->setEnabled(!bEmpty);
    f_save->setEnabled((!bEmpty) && (!saved()));
    f_share->f_export->setEnabled(!bEmpty);
}
void VehicleMission::updateSize()
{
    if (blockSizeUpdate)
        return;
    int cnt = 0;
    foreach (MissionGroup *group, groups) {
        cnt += group->size();
    }
    setMissionSize(cnt);
}
void VehicleMission::updateStatus()
{
    QString s = f_title->text();
    int sz = missionSize();
    if (sz <= 0 && s.isEmpty())
        s = title();
    else if (s.isEmpty())
        s = QString("(%1)").arg(tr("No title"));
    if (sz > 0)
        s.append(QString(" [%1]").arg(sz));
    setValue(s.simplified());
}

void VehicleMission::updateStartPath()
{
    if (f_waypoints->size() <= 0)
        return;
    static_cast<Waypoint *>(f_waypoints->child(0))->updatePath();
}

QGeoRectangle VehicleMission::boundingGeoRectangle() const
{
    QGeoRectangle r;
    foreach (MissionGroup *group, groups) {
        for (int i = 0; i < group->size(); ++i) {
            QGeoRectangle re = static_cast<MissionItem *>(group->child(i))->boundingGeoRectangle();
            r = r.isValid() ? r.united(re) : re;
        }
    }
    return r;
}

void VehicleMission::clearMission()
{
    f_title->setValue("");
    setSite("");
    setMissionSize(0);
    blockSizeUpdate = true;
    foreach (MissionGroup *group, groups) {
        group->f_clear->trigger();
    }
    blockSizeUpdate = false;
    setModified(false);

    App::jsync(this);
}

QJsonValue VehicleMission::toJson() const
{
    QJsonObject json = Fact::toJson().toObject();

    json.insert("format", MISSION_FORMAT);
    json.insert("exported", QDateTime::currentDateTime().toString(Qt::RFC2822Date));
    json.insert("version", App::version());

    json.insert("title", json[f_title->name()]);
    json.remove(f_title->name());

    if (!site().isEmpty())
        json.insert("site", site());

    QGeoCoordinate c = coordinate();
    json.insert("lat", c.latitude());
    json.insert("lon", c.longitude());

    QString title = f_title->text().simplified();
    if (vehicle->isIdentified()) {
        QString s = vehicle->title();
        json.insert("callsign", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    if (f_runways->size() > 0) {
        QString s = f_runways->child(0)->text();
        json.insert("runway", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    title.replace('-', ' ');
    title.replace('_', ' ');
    title = title.simplified();
    f_title->setValue(title);

    //details
    QGeoRectangle rect = boundingGeoRectangle();
    json.insert("topLeftLat", rect.topLeft().latitude());
    json.insert("topLeftLon", rect.topLeft().longitude());
    json.insert("bottomRightLat", rect.bottomRight().latitude());
    json.insert("bottomRightLon", rect.bottomRight().longitude());
    json.insert("distance", (qint64) f_waypoints->distance());

    //generate hash
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(QJsonDocument(json.value("rw").toArray()).toJson(QJsonDocument::Compact));
    h.addData(QJsonDocument(json.value("wp").toArray()).toJson(QJsonDocument::Compact));
    h.addData(QJsonDocument(json.value("tw").toArray()).toJson(QJsonDocument::Compact));
    h.addData(QJsonDocument(json.value("pi").toArray()).toJson(QJsonDocument::Compact));
    QString hash = h.result().toHex().toUpper();
    json.insert("hash", hash);

    return json;
}
void VehicleMission::fromJson(const QJsonValue json)
{
    clearMission();

    setSite(json["site"].toString());

    f_title->setValue(json["title"].toString());

    blockSizeUpdate = true;
    for (auto i : groups) {
        i->fromJson(json[i->name()]);
    }
    blockSizeUpdate = false;

    backup();
    updateSize();

    App::jsync(this);
}

void VehicleMission::hashData(QCryptographicHash *h) const
{
    foreach (MissionGroup *group, groups) {
        group->hashData(h);
    }
}

void VehicleMission::test(int n)
{
    if (f_waypoints->size() <= 0)
        return;
    Waypoint *w = static_cast<Waypoint *>(f_waypoints->facts().last());
    QGeoCoordinate p(w->f_latitude->value().toDouble(), w->f_longitude->value().toDouble());
    double hdg = QRandomGenerator::global()->bounded(360.0);
    for (int i = 0; i < n; ++i) {
        hdg += QRandomGenerator::global()->bounded(200.0) - 100.0;
        p = p.atDistanceAndAzimuth(100 + QRandomGenerator::global()->bounded(10000.0), hdg);
        f_waypoints->addObject(p);
    }
}

void VehicleMission::missionReceived(QJsonValue json)
{
    clearMission();
    //qDebug() << json;

    fromJson(json);

    if (empty()) {
        vehicle->message(tr("Empty mission received from vehicle"), AppNotify::Warning);
    } else {
        emit missionAvailable();
        emit missionDownloaded();
        storage->saveMission();
        vehicle->message(QString("%1: %2").arg(tr("Mission received")).arg(text()),
                         AppNotify::Important);
    }
    backup();
}
void VehicleMission::missionDataError()
{
    vehicle->message(tr("Error in mission data from vehicle"), AppNotify::Error);
    clearMission();
}

void VehicleMission::uploadMission()
{
    vehicle->message(QString("%1: %2...").arg(tr("Uploading mission")).arg(text()), AppNotify::Info);
    // vehicle->protocol()->mission->setActive(true);
    vehicle->protocol()->mission()->updateMission(toJson());
    f_save->trigger();
}
void VehicleMission::downloadMission()
{
    vehicle->message(QString("%1...").arg(tr("Requesting mission")), AppNotify::Info);
    // vehicle->protocol()->mission->setActive(true);
    vehicle->protocol()->mission()->requestMission();
}

QGeoCoordinate VehicleMission::startPoint() const
{
    return m_startPoint;
}
void VehicleMission::setStartPoint(const QGeoCoordinate &v)
{
    if (m_startPoint == v)
        return;
    m_startPoint = v;
    emit startPointChanged();
}
double VehicleMission::startHeading() const
{
    return m_startHeading;
}
void VehicleMission::setStartHeading(const double &v)
{
    if (m_startHeading == v)
        return;
    m_startHeading = v;
    emit startHeadingChanged();
}
double VehicleMission::startLength() const
{
    return m_startLength;
}
void VehicleMission::setStartLength(const double &v)
{
    if (m_startLength == v)
        return;
    m_startLength = v;
    emit startLengthChanged();
}
MissionListModel *VehicleMission::listModel() const
{
    return m_listModel;
}
int VehicleMission::missionSize() const
{
    return m_missionSize;
}
void VehicleMission::setMissionSize(const int v)
{
    if (m_missionSize == v)
        return;
    m_missionSize = v;
    emit missionSizeChanged();
    setEmpty(v <= 0);
}
bool VehicleMission::empty() const
{
    return m_empty;
}
void VehicleMission::setEmpty(const bool v)
{
    if (m_empty == v)
        return;
    m_empty = v;
    emit emptyChanged();
}
QGeoCoordinate VehicleMission::coordinate() const
{
    return m_coordinate;
}
void VehicleMission::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
}
QString VehicleMission::site() const
{
    return m_site;
}
void VehicleMission::setSite(const QString &v)
{
    if (m_site == v)
        return;
    m_site = v;
    emit siteChanged();
}
bool VehicleMission::synced() const
{
    return m_synced;
}
void VehicleMission::setSynced(const bool v)
{
    if (m_synced == v)
        return;
    m_synced = v;
    emit syncedChanged();
}
bool VehicleMission::saved() const
{
    return m_saved;
}
void VehicleMission::setSaved(const bool v)
{
    if (m_saved == v)
        return;
    m_saved = v;
    emit savedChanged();
}
Fact *VehicleMission::selectedItem() const
{
    return m_selectedItem;
}
void VehicleMission::setSelectedItem(Fact *v)
{
    if (m_selectedItem == v)
        return;
    m_selectedItem = v;
    emit selectedItemChanged();
}
