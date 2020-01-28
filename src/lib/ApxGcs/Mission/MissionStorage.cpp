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
#include "MissionStorage.h"
#include "LookupMissions.h"
#include "Poi.h"
#include "Runway.h"
#include "Taxiway.h"
#include "VehicleMission.h"
#include "Waypoint.h"
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Database/MissionsDB.h>
#include <Dictionary/DictMission.h>
#include <Vehicles/VehicleSelect.h>
#include <Vehicles/Vehicles.h>

#include "MissionShare.h"
//=============================================================================
MissionStorage::MissionStorage(VehicleMission *mission)
    : QObject(mission)
    , mission(mission)
{
    connect(&evtUpdateSite, &DelayedEvent::triggered, this, &MissionStorage::dbFindSite);
    connect(mission, &VehicleMission::coordinateChanged, &evtUpdateSite, &DelayedEvent::schedule);
}
//=============================================================================
void MissionStorage::saveMission()
{
    dbHash.clear();
    //collect details
    QVariantMap details;
    QString title = mission->f_title->text().simplified();
    if (!(mission->vehicle->isLocal() || mission->vehicle->isReplay())) {
        QString s = mission->vehicle->callsign();
        details.insert("callsign", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    if (mission->f_runways->size() > 0) {
        QString s = mission->f_runways->child(0)->text();
        details.insert("runway", s);
        title.remove(s, Qt::CaseInsensitive);
    }
    title.replace('-', ' ');
    title.replace('_', ' ');
    title = title.simplified();
    mission->f_title->setValue(title);

    //details
    QGeoRectangle rect = mission->boundingGeoRectangle();
    details.insert("topLeftLat", rect.topLeft().latitude());
    details.insert("topLeftLon", rect.topLeft().longitude());
    details.insert("bottomRightLat", rect.bottomRight().latitude());
    details.insert("bottomRightLon", rect.bottomRight().longitude());
    details.insert("distance", mission->f_waypoints->distance());

    //request
    DBReqMissionsSave *req = new DBReqMissionsSave(saveToDict(), details);
    connect(req,
            &DBReqMissionsSave::missionHash,
            this,
            &MissionStorage::dbSaved,
            Qt::QueuedConnection);
    connect(req,
            &DBReqMissionsSave::siteFound,
            this,
            &MissionStorage::dbSiteFound,
            Qt::QueuedConnection);
    req->exec();
}
void MissionStorage::dbSaved(QString hash)
{
    bool bHashUpdate = dbHash == hash;
    dbHash = hash;
    emit saved();
    if (bHashUpdate) {
        mission->vehicle->message(tr("Mission record updated"));
    } else {
        mission->vehicle->message(tr("Mission saved"));
    }
    mission->backup();
}
//=============================================================================
void MissionStorage::dbFindSite()
{
    QGeoCoordinate c(mission->coordinate());
    if (!c.isValid()) {
        mission->setSite("");
        return;
    }
    DBReqMissionsFindSite *req = new DBReqMissionsFindSite(c.latitude(), c.longitude());
    connect(req,
            &DBReqMissionsFindSite::siteFound,
            this,
            &MissionStorage::dbSiteFound,
            Qt::QueuedConnection);
    req->exec();
}
void MissionStorage::dbSiteFound(quint64 siteID, QString site)
{
    Q_UNUSED(siteID)
    mission->setSite(site);
}
//=============================================================================
void MissionStorage::loadMission(QString hash)
{
    //qDebug()<<"load mission"<<missionID;
    if (hash.isEmpty())
        return;
    if ((!mission->modified()) && mission->missionSize() > 0 && dbHash == hash)
        return;
    dbHash = hash;

    DBReqMissionsLoad *req = new DBReqMissionsLoad(hash);
    connect(req, &DBReqMissionsLoad::loaded, this, &MissionStorage::dbLoaded, Qt::QueuedConnection);
    req->exec();
}
void MissionStorage::dbLoaded(QVariantMap info, QVariantMap details, DictMission::Mission data)
{
    Q_UNUSED(details)
    //qDebug()<<"mission loaded"<<m.values.size();
    mission->clearMission();
    mission->setSite(info.value("site").toString());

    dbHash = info.value("hash").toString();
    mission->f_title->setValue(data.title);

    loadFromDict(data);

    if (mission->missionSize() > 0) {
        emit loaded();
    }
    QString s(tr("Mission loaded"));
    if (data.title.isEmpty())
        mission->vehicle->message(s);
    else
        mission->vehicle->message(s.append(": ").append(data.title));
}
//=============================================================================
//=============================================================================
DictMission::Mission MissionStorage::saveToDict() const
{
    DictMission::Mission d;
    saveItemsToDict(d.runways, mission->f_runways);
    saveItemsToDict(d.waypoints, mission->f_waypoints);
    saveItemsToDict(d.taxiways, mission->f_taxiways);
    saveItemsToDict(d.pois, mission->f_pois);

    d.title = mission->f_title->text().simplified();

    QGeoCoordinate c = mission->coordinate();
    d.lat = c.latitude();
    d.lon = c.longitude();
    return d;
}
void MissionStorage::loadFromDict(DictMission::Mission d)
{
    mission->blockSizeUpdate = true;
    loadItemsFromDict(d.runways, mission->f_runways);
    loadItemsFromDict(d.waypoints, mission->f_waypoints);
    loadItemsFromDict(d.taxiways, mission->f_taxiways);
    loadItemsFromDict(d.pois, mission->f_pois);

    mission->blockSizeUpdate = false;
    mission->backup();
    mission->updateSize();
}
//=============================================================================
//=============================================================================
void MissionStorage::saveItemsToDict(QList<DictMission::Item> &items, const MissionGroup *g) const
{
    for (int i = 0; i < g->size(); ++i) {
        MissionItem *f = static_cast<MissionItem *>(g->child(i));
        DictMission::Item e;
        e.lat = f->f_latitude->value().toDouble();
        e.lon = f->f_longitude->value().toDouble();
        for (int j = 0; j < f->size(); ++j) {
            MissionItem *fv = static_cast<MissionItem *>(f->child(j));
            //skip main fields
            if (fv == f->f_order)
                continue;
            if (fv == f->f_latitude)
                continue;
            if (fv == f->f_longitude)
                continue;
            e.details.insert(fv->objectName(), fv->text());
        }
        items.append(e);
    }
}
void MissionStorage::loadItemsFromDict(const QList<DictMission::Item> &items, MissionGroup *g) const
{
    for (int i = 0; i < items.size(); ++i) {
        const DictMission::Item &e = items.at(i);
        MissionItem *f = g->createObject();
        f->f_latitude->setValue(e.lat);
        f->f_longitude->setValue(e.lon);
        foreach (QString key, e.details.keys()) {
            Fact *fv = f->child(key);
            if (!fv) {
                qWarning() << "missing value" << g->title() << f->title() << key;
                continue;
            }
            fv->setValue(e.details.value(key));
        }
    }
}
//=============================================================================
//=============================================================================
