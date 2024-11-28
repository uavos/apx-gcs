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
#include "MissionField.h"
#include "MissionGroup.h"
#include "MissionListModel.h"
#include "MissionShare.h"
#include "MissionTools.h"

#include "Area.h"
#include "Poi.h"
#include "Runway.h"
#include "Taxiway.h"
#include "Waypoint.h"

#include <App/App.h>
#include <Nodes/Nodes.h>
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
    f_upload
        = new Fact(this, "upload", tr("Upload"), tr("Upload to vehicle"), Action | Apply, "upload");
    connect(f_upload, &Fact::triggered, this, &VehicleMission::uploadMission);

    f_request = new Fact(this,
                         "request",
                         tr("Request"),
                         tr("Download from vehicle"),
                         Action | IconOnly,
                         "download");
    connect(f_request, &Fact::triggered, this, &VehicleMission::downloadMission);

    f_clear = new Fact(this, "clear", tr("Clear"), tr("Clear mission"), Action | Remove | IconOnly);
    connect(f_clear, &Fact::triggered, this, &VehicleMission::clearMission);

    //tools actions
    f_tools = new MissionTools(this, Action | IconOnly);

    f_share = new MissionShare(this, f_tools);
    connect(f_share, &Share::imported, this, [this]() { setSynced(false); });
    connect(f_share, &Share::exported, this, [this]() { backup(); });

    // tools [...] is the last item in list
    f_tools->move(size() - 1);

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
    connect(this, &VehicleMission::syncedChanged, this, &VehicleMission::updateActions);

    connect(this, &VehicleMission::startPointChanged, this, &VehicleMission::updateStartPath);
    connect(this, &VehicleMission::startHeadingChanged, this, &VehicleMission::updateStartPath);
    connect(this, &VehicleMission::startLengthChanged, this, &VehicleMission::updateStartPath);

    //sync and saved status behavior
    connect(this, &Fact::modifiedChanged, this, [this]() {
        if (modified()) {
            setSynced(false);
        }
    });
    connect(this, &VehicleMission::emptyChanged, this, [this]() {
        if (empty()) {
            setSynced(false);
        }
    });
    connect(this, &VehicleMission::missionUploaded, this, [this]() {
        backup();
        setSynced(true);
    });

    //protocols
    if (vehicle->protocol()) {
        connect(vehicle->protocol()->mission(),
                &PMission::missionUpdated,
                this,
                &VehicleMission::missionUploaded);

        connect(vehicle->protocol()->mission(),
                &PMission::missionReceived,
                this,
                &VehicleMission::missionReceived);

        if (vehicle->isIdentified()) {
            connect(vehicle->protocol()->mission(), &PMission::missionAvailable, this, [this]() {
                if (empty())
                    vehicle->protocol()->mission()->requestMission();
            });
        }
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

QVariant VehicleMission::toVariant()
{
    auto m = Fact::toVariant().value<QVariantMap>();

    m.insert("format", MISSION_FORMAT);
    m.insert("exported", QDateTime::currentDateTime().toString(Qt::RFC2822Date));
    m.insert("version", App::version());

    m.remove(f_title->name());

    if (!site().isEmpty())
        m.insert("site", site());

    QGeoCoordinate c = coordinate();
    m.insert("lat", c.latitude());
    m.insert("lon", c.longitude());

    QString title = f_title->text().simplified();
    if (vehicle->isIdentified()) {
        QString s = vehicle->title();
        m.insert("callsign", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    if (f_runways->size() > 0) {
        QString s = f_runways->child(0)->text();
        m.insert("runway", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    title.replace('-', ' ');
    title.replace('_', ' ');
    title = title.simplified();
    if (!title.isEmpty())
        m.insert("title", title);

    //details
    QGeoRectangle rect = boundingGeoRectangle();
    m.insert("topLeftLat", rect.topLeft().latitude());
    m.insert("topLeftLon", rect.topLeft().longitude());
    m.insert("bottomRightLat", rect.bottomRight().latitude());
    m.insert("bottomRightLon", rect.bottomRight().longitude());
    m.insert("distance", f_waypoints->distance());

    //generate hash
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QJsonDocument::fromVariant(m.value("rw")).toJson(QJsonDocument::Compact));
    hash.addData(QJsonDocument::fromVariant(m.value("wp")).toJson(QJsonDocument::Compact));
    hash.addData(QJsonDocument::fromVariant(m.value("tw")).toJson(QJsonDocument::Compact));
    hash.addData(QJsonDocument::fromVariant(m.value("pi")).toJson(QJsonDocument::Compact));
    m.insert("hash", hash.result().toHex().toUpper());

    m.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    return m;
}
void VehicleMission::fromVariant(const QVariant &var)
{
    clearMission();

    auto m = var.value<QVariantMap>();
    setSite(m.value("site").toString());

    f_title->setValue(m.value("title").toString());

    blockSizeUpdate = true;
    for (auto i : groups) {
        i->fromVariant(m.value(i->name()));
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

void VehicleMission::missionReceived(QVariant var)
{
    clearMission();
    //qDebug() << json;

    fromVariant(var);

    if (empty()) {
        vehicle->message(tr("Empty mission received from vehicle"), AppNotify::Warning);
    } else {
        emit missionDownloaded();
        QString s = QString("%1: %2").arg(tr("Mission received")).arg(text());
        auto node_uid = var.value<QVariantMap>().value("node_uid").toString();
        if (!node_uid.isEmpty()) {
            auto node = vehicle->f_nodes->node(node_uid);
            if (node) {
                s.append(QString(" (%1)").arg(node->title()));
            }
        }
        vehicle->message(s, AppNotify::Important);
    }
    setSynced(true);
    backup();
}

void VehicleMission::uploadMission()
{
    if (!vehicle->protocol())
        return;
    vehicle->message(QString("%1: %2...").arg(tr("Uploading mission")).arg(text()), AppNotify::Info);
    vehicle->protocol()->mission()->updateMission(toVariant());
}
void VehicleMission::downloadMission()
{
    if (!vehicle->protocol())
        return;
    vehicle->message(QString("%1...").arg(tr("Requesting mission")), AppNotify::Info);
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
