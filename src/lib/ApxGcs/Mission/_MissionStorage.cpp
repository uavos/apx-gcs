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
#include "LookupMissions.h"
#include "MissionStorage.h"
#include "Poi.h"
#include "Runway.h"
#include "Taxiway.h"
#include "VehicleMission.h"
#include "Waypoint.h"

#include <App/AppLog.h>
#include <Vehicles/VehicleSelect.h>
#include <Vehicles/Vehicles.h>

MissionStorage::MissionStorage(VehicleMission *mission)
    : QObject(mission)
    , mission(mission)
{
    connect(&evtUpdateSite, &DelayedEvent::triggered, this, &MissionStorage::dbFindSite);
    connect(mission, &VehicleMission::coordinateChanged, &evtUpdateSite, &DelayedEvent::schedule);
}

void MissionStorage::saveMission()
{
    if (mission->empty()) {
        mission->vehicle->message(tr("Mission is empty"));
        return;
    }

    /*dbHash.clear();
    DBReqMissionsSave *req = new DBReqMissionsSave(mission->toVariant());
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
    req->exec();*/
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
void MissionStorage::dbLoaded(QVariant var)
{
    auto m = var.value<QVariantMap>();
    dbHash = m.value("hash").toString();
    // qDebug() << "mission load:" << dbHash;

    mission->fromVariant(var);

    if (mission->missionSize() > 0) {
        emit loaded();
    }
    QString s(tr("Mission loaded"));
    QString title = mission->f_title->text();
    if (title.isEmpty())
        mission->vehicle->message(s);
    else
        mission->vehicle->message(s.append(": ").append(title));
}
