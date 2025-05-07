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
#include "UnitMission.h"
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
#include <ApxMisc/JsonHelpers.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Nodes/Nodes.h>
#include <QQmlEngine>

#define MISSION_FORMAT "11"

UnitMission::UnitMission(Unit *parent)
    : Fact(parent, "mission", "Mission", tr("Unit mission"), Group | ModifiedGroup, "ship-wheel")
    , unit(parent)
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

    connect(f_title, &Fact::textChanged, this, &UnitMission::updateStatus);
    connect(this, &UnitMission::missionSizeChanged, this, &UnitMission::updateStatus);

    //groups of items
    f_runways = new Runways(this,
                            "rw",
                            tr("Runways"),
                            tr("Takeoff and Landing"),
                            unit->f_mandala->fact("cmd.proc.rw"));
    f_waypoints = new Waypoints(this,
                                "wp",
                                tr("Waypoints"),
                                "",
                                unit->f_mandala->fact("cmd.proc.wp"));
    f_pois = new Pois(this,
                      "pi",
                      tr("Points"),
                      tr("Points of Interest"),
                      unit->f_mandala->fact("cmd.proc.pi"));
    f_taxiways = new Taxiways(this, "tw", tr("Taxiways"), "", unit->f_mandala->fact("cmd.proc.wp"));
    f_areas = new Areas(this, "area", tr("Area"), tr("Airspace definitions"));

    for (auto group : groups) {
        connect(group, &Fact::sizeChanged, this, &UnitMission::updateSize, Qt::QueuedConnection);
    }

    //actions
    f_upload = new Fact(this, "upload", tr("Upload"), tr("Upload to unit"), Action | Apply, "upload");
    connect(f_upload, &Fact::triggered, this, &UnitMission::uploadMission);

    f_request = new Fact(this,
                         "request",
                         tr("Request"),
                         tr("Download from unit"),
                         Action | IconOnly,
                         "download");
    connect(f_request, &Fact::triggered, this, &UnitMission::downloadMission);

    f_clear = new Fact(this, "clear", tr("Clear"), tr("Clear mission"), Action | Remove | IconOnly);
    connect(f_clear, &Fact::triggered, this, &UnitMission::clearMission);

    //tools actions
    f_tools = new MissionTools(this, Action | IconOnly);

    f_share = new MissionShare(this, f_tools);
    connect(f_share, &Share::imported, this, [this]() { setSynced(false); });
    connect(f_share, &Share::exported, this, [this]() { backup(); });

    // tools [...] is the last item in list
    f_tools->move(size() - 1);

    //App::jsync(f_tools);

    for (auto a : actions()) {
        connect(static_cast<Fact *>(a), &Fact::enabledChanged, this, &UnitMission::actionsUpdated);
    }

    //internal
    m_listModel = new MissionListModel(this);

    connect(this, &UnitMission::emptyChanged, this, &UnitMission::updateActions);
    connect(this, &UnitMission::syncedChanged, this, &UnitMission::updateActions);

    connect(this, &UnitMission::startPointChanged, this, &UnitMission::updateStartPath);
    connect(this, &UnitMission::startHeadingChanged, this, &UnitMission::updateStartPath);
    connect(this, &UnitMission::startLengthChanged, this, &UnitMission::updateStartPath);

    //sync and saved status behavior
    connect(this, &Fact::modifiedChanged, this, [this]() {
        if (modified()) {
            setSynced(false);
        }
    });
    connect(this, &UnitMission::emptyChanged, this, [this]() {
        if (empty()) {
            setSynced(false);
        }
    });
    connect(this, &UnitMission::missionUploaded, this, [this]() {
        backup();
        setSynced(true);
    });

    //protocols
    if (unit->protocol()) {
        connect(unit->protocol()->mission(),
                &PMission::missionUpdated,
                this,
                &UnitMission::missionUploaded);

        connect(unit->protocol()->mission(),
                &PMission::missionReceived,
                this,
                &UnitMission::missionReceived);

        if (unit->isIdentified()) {
            connect(unit->protocol()->mission(), &PMission::missionAvailable, this, [this]() {
                if (empty())
                    unit->protocol()->mission()->requestMission();
            });
        }
    }

    //reset and update
    clearMission();
    updateActions();
    updateStatus();

    qmlRegisterUncreatableType<UnitMission>("APX.Mission", 1, 0, "Mission", "Reference only");
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

void UnitMission::updateActions()
{
    bool bEmpty = empty();
    f_upload->setEnabled(!bEmpty);
    f_clear->setEnabled(!bEmpty);
}
void UnitMission::updateSize()
{
    if (blockSizeUpdate)
        return;
    int cnt = 0;
    for (auto group : groups) {
        cnt += group->size();
    }
    setMissionSize(cnt);
}
void UnitMission::updateStatus()
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

void UnitMission::updateStartPath()
{
    if (f_waypoints->size() <= 0)
        return;
    static_cast<Waypoint *>(f_waypoints->child(0))->updatePath();
}

QGeoRectangle UnitMission::boundingGeoRectangle() const
{
    QGeoRectangle r;
    for (auto group : groups) {
        for (int i = 0; i < group->size(); ++i) {
            QGeoRectangle re = static_cast<MissionItem *>(group->child(i))->boundingGeoRectangle();
            r = r.isValid() ? r.united(re) : re;
        }
    }
    return r;
}

void UnitMission::clearMission()
{
    f_title->setValue("");
    setSite("");
    setMissionSize(0);
    blockSizeUpdate = true;
    for (auto group : groups) {
        group->f_clear->trigger();
    }
    blockSizeUpdate = false;
    setModified(false);

    App::jsync(this);
}

QJsonValue UnitMission::toJson()
{
    auto jso = Fact::toJson().toObject();

    jso.insert("format", MISSION_FORMAT);
    jso.insert("exported", QDateTime::currentDateTime().toString(Qt::RFC2822Date));
    jso.insert("version", App::version());

    jso.remove(f_title->name());

    if (!site().isEmpty())
        jso.insert("site", site());

    QGeoCoordinate c = coordinate();
    jso.insert("lat", c.latitude());
    jso.insert("lon", c.longitude());

    QString title = f_title->text().simplified();
    if (unit->isIdentified()) {
        QString s = unit->title();
        jso.insert("callsign", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    if (f_runways->size() > 0) {
        QString s = f_runways->child(0)->text();
        jso.insert("runway", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    title.replace('-', ' ');
    title.replace('_', ' ');
    title = title.simplified();
    if (!title.isEmpty())
        jso.insert("title", title);

    //details
    QGeoRectangle rect = boundingGeoRectangle();
    jso.insert("topLeftLat", rect.topLeft().latitude());
    jso.insert("topLeftLon", rect.topLeft().longitude());
    jso.insert("bottomRightLat", rect.bottomRight().latitude());
    jso.insert("bottomRightLon", rect.bottomRight().longitude());
    jso.insert("distance", (qint64) f_waypoints->distance());

    //generate hash
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QJsonDocument(jso.value("rw").toArray()).toJson(QJsonDocument::Compact));
    hash.addData(QJsonDocument(jso.value("wp").toArray()).toJson(QJsonDocument::Compact));
    hash.addData(QJsonDocument(jso.value("tw").toArray()).toJson(QJsonDocument::Compact));
    hash.addData(QJsonDocument(jso.value("pi").toArray()).toJson(QJsonDocument::Compact));
    jso.insert("hash", QString(hash.result().toHex().toUpper()));

    jso.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());

    jso = json::remove_empty(json::fix_numbers(jso), true);

    return jso;
}
void UnitMission::fromJson(const QJsonValue &jsv)
{
    clearMission();

    const auto jso = jsv.toObject();
    setSite(jso.value("site").toString());

    f_title->setValue(jso.value("title").toString());

    blockSizeUpdate = true;
    for (auto i : groups) {
        i->fromJson(jso.value(i->name()));
    }
    blockSizeUpdate = false;

    backup();
    updateSize();

    App::jsync(this);
}

void UnitMission::hashData(QCryptographicHash *h) const
{
    for (auto group : groups) {
        group->hashData(h);
    }
}

void UnitMission::test(int n)
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

void UnitMission::missionReceived(QVariant var)
{
    clearMission();
    //qDebug() << json;

    fromJson(QJsonValue::fromVariant(var));

    if (empty()) {
        unit->message(tr("Empty mission received"), AppNotify::Warning);
    } else {
        emit missionDownloaded();
        QString s = QString("%1: %2").arg(tr("Mission received")).arg(text());
        auto node_uid = var.value<QVariantMap>().value("node_uid").toString();
        if (!node_uid.isEmpty()) {
            auto node = unit->f_nodes->node(node_uid);
            if (node) {
                s.append(QString(" (%1)").arg(node->title()));
            }
        }
        unit->message(s, AppNotify::Important);
    }
    setSynced(true);
    backup();
}

void UnitMission::uploadMission()
{
    if (!unit->protocol())
        return;
    unit->message(QString("%1: %2...").arg(tr("Uploading mission")).arg(text()), AppNotify::Info);
    unit->protocol()->mission()->updateMission(toJson());
}
void UnitMission::downloadMission()
{
    if (!unit->protocol())
        return;
    unit->message(QString("%1...").arg(tr("Requesting mission")), AppNotify::Info);
    unit->protocol()->mission()->requestMission();
}

QGeoCoordinate UnitMission::startPoint() const
{
    return m_startPoint;
}
void UnitMission::setStartPoint(const QGeoCoordinate &v)
{
    if (m_startPoint == v)
        return;
    m_startPoint = v;
    emit startPointChanged();
}
double UnitMission::startHeading() const
{
    return m_startHeading;
}
void UnitMission::setStartHeading(const double &v)
{
    if (m_startHeading == v)
        return;
    m_startHeading = v;
    emit startHeadingChanged();
}
double UnitMission::startLength() const
{
    return m_startLength;
}
void UnitMission::setStartLength(const double &v)
{
    if (m_startLength == v)
        return;
    m_startLength = v;
    emit startLengthChanged();
}
MissionListModel *UnitMission::listModel() const
{
    return m_listModel;
}
int UnitMission::missionSize() const
{
    return m_missionSize;
}
void UnitMission::setMissionSize(const int v)
{
    if (m_missionSize == v)
        return;
    m_missionSize = v;
    emit missionSizeChanged();
    setEmpty(v <= 0);
}
bool UnitMission::empty() const
{
    return m_empty;
}
void UnitMission::setEmpty(const bool v)
{
    if (m_empty == v)
        return;
    m_empty = v;
    emit emptyChanged();
}
QGeoCoordinate UnitMission::coordinate() const
{
    return m_coordinate;
}
void UnitMission::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
}
QString UnitMission::site() const
{
    return m_site;
}
void UnitMission::setSite(const QString &v)
{
    if (m_site == v)
        return;
    m_site = v;
    emit siteChanged();
}
bool UnitMission::synced() const
{
    return m_synced;
}
void UnitMission::setSynced(const bool v)
{
    if (m_synced == v)
        return;
    m_synced = v;
    emit syncedChanged();
}
Fact *UnitMission::selectedItem() const
{
    return m_selectedItem;
}
void UnitMission::setSelectedItem(Fact *v)
{
    if (m_selectedItem == v)
        return;
    m_selectedItem = v;
    emit selectedItemChanged();
}
