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
#include "LookupMissions.h"
#include <App/AppRoot.h>
#include <Database/Database.h>
#include <Database/MissionsDB.h>

#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
LookupMissions::LookupMissions(VehicleMission *mission, Fact *parent, Flags flags)
    : DatabaseLookup(parent,
                     "load",
                     tr("Load mission"),
                     tr("Database lookup"),
                     Database::instance()->missions,
                     flags)
    , mission(mission)
{
    setOpt("pos", QPointF(0, 1));

    connect(this, &DatabaseLookup::itemTriggered, this, &LookupMissions::loadItem);
}
//=============================================================================
void LookupMissions::loadItem(QVariantMap modelData)
{
    QString hash = modelData.value("hash").toString();
    if (hash.isEmpty())
        return;
    if (mission)
        mission->storage->loadMission(hash);
    else
        Vehicles::instance()->current()->f_mission->storage->loadMission(hash);
}
//=============================================================================
bool LookupMissions::fixItemDataThr(QVariantMap *item)
{
    QString title = item->value("title").toString();
    QString cnt = QString::number(item->value("cnt").toUInt());
    QString callsign = item->value("callsign").toString();
    QString site = item->value("site").toString();
    QString runway = item->value("runway").toString();
    //title=QString("%1 [%2]").arg(title).arg(cnt);
    if (!site.isEmpty())
        title.prepend(title.isEmpty() ? "" : " - ").prepend(site);
    if (!callsign.isEmpty())
        title.prepend(title.isEmpty() ? "" : " - ").prepend(callsign);
    if (!runway.isEmpty())
        title.append(title.isEmpty() ? "" : " - ").append(runway);

    (*item)["title"] = title;
    QString sdescr = QDateTime::fromMSecsSinceEpoch(item->value("time").toLongLong())
                         .toString("d MMM yyyy hh:mm");
    if (item->contains("dist") && reqPoint.isValid()) {
        QGeoCoordinate p(item->value("lat").toDouble(), item->value("lon").toDouble());
        sdescr.append(QString(" (%1)").arg(AppRoot::distanceToString(reqPoint.distanceTo(p))));
    }
    item->insert("descr", sdescr);
    QString status = QString("%1").arg(
        AppRoot::distanceToString(item->value("distance").toULongLong()));
    status.append(" ").append(cnt);
    item->insert("status", status);
    return true;
}
//=============================================================================
//=============================================================================
//=============================================================================
void LookupMissions::defaultLookup()
{
    const QString s = QString("%%%1%%").arg(filter());
    query("SELECT * FROM Missions"
          " LEFT JOIN MissionDetails ON MissionDetails.missionID=Missions.key"
          " LEFT JOIN (SELECT key, title AS site FROM Sites) AS Sites  ON Missions.siteID=Sites.key"
          " LEFT JOIN (SELECT missionID, COUNT(*) AS cnt FROM Waypoints GROUP BY missionID) AS "
          "Waypoints ON Waypoints.missionID=Missions.key"
          " WHERE"
          " ( title LIKE ? OR callsign LIKE ? OR site LIKE ? OR runway LIKE ? )"
          " ORDER BY time DESC",
          QVariantList() << s << s << s << s);
    setDescr(tr("Ordered by time"));
}
void LookupMissions::dbLookupMissionsByArea(QGeoCoordinate c, QString siteName)
{
    const QString s = QString("%%%1%%").arg(filter());
    reqPoint = c;
    QVariantList v;
    v << c.latitude();
    v << c.latitude();
    v << std::pow(std::cos(qDegreesToRadians(c.latitude())), 2);
    v << c.longitude();
    v << c.longitude();
    query("SELECT *, ((?-lat)*(?-lat)*?)+((?-lon)*(?-lon)) AS dist FROM Missions"
          " LEFT JOIN MissionDetails ON MissionDetails.missionID=Missions.key"
          " LEFT JOIN (SELECT key, title AS site FROM Sites) AS Sites  ON Missions.siteID=Sites.key"
          " LEFT JOIN (SELECT missionID, COUNT(*) AS cnt FROM Waypoints GROUP BY missionID) AS "
          "Waypoints ON Waypoints.missionID=Missions.key"
          " WHERE dist<200 AND"
          " ( title LIKE ? OR callsign LIKE ? OR site LIKE ? OR runway LIKE ? )"
          " ORDER BY dist ASC, time DESC"
          " LIMIT 200",
          v << s << s << s << s);
    QString sdescr = tr("Closest to site");
    setDescr(siteName.isEmpty() ? sdescr : (sdescr.append(QString(" '%1'").arg(siteName))));
}
//=============================================================================
//=============================================================================
