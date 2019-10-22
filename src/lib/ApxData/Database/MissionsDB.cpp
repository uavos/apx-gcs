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
#include "MissionsDB.h"
#include "Database.h"
#include <App/AppDirs.h>
//=============================================================================
MissionsDB::MissionsDB(QObject *parent, QString sessionName)
    : DatabaseSession(parent, AppDirs::db().absoluteFilePath("missions.db"), sessionName)
{
    new DBReqMakeTable(this,
                       "Missions",
                       QStringList()
                           << "key INTEGER PRIMARY KEY NOT NULL"
                           << "hash TEXT NOT NULL UNIQUE"
                           << "time INTEGER DEFAULT 0" //access time
                           << "title TEXT"
                           << "lat REAL"
                           << "lon REAL"
                           << "siteID INTEGER" //auto assigned
                           << "FOREIGN KEY(siteID) REFERENCES Sites(key) ON DELETE SET NULL");
    new DBReqMakeIndex(this, "Missions", "hash", true);
    new DBReqMakeIndex(this, "Missions", "time", false);
    new DBReqMakeIndex(this, "Missions", "title", false);

    new DBReqMakeTable(this,
                       "Sites",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "title TEXT NOT NULL"
                                     << "lat REAL NOT NULL"
                                     << "lon REAL NOT NULL");
    new DBReqMakeIndex(this, "Sites", "title", true);
    new DBReqMakeIndex(this, "Sites", "lat,lon", true);

    new DBReqMakeTable(this,
                       "MissionDetails",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "missionID INTEGER NOT NULL"
                                     << "topLeftLat REAL"
                                     << "topLeftLon REAL"
                                     << "bottomRightLat REAL"
                                     << "bottomRightLon REAL"
                                     << "distance INTEGER"
                                     << "callsign TEXT"
                                     << "runway TEXT");
    new DBReqMakeIndex(this, "MissionDetails", "missionID", true);
    new DBReqMakeIndex(this, "MissionDetails", "callsign", false);
    new DBReqMakeIndex(this, "MissionDetails", "runway", false);

    //mission elements
    new DBReqMakeTable(this,
                       "Runways",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "missionID INTEGER NOT NULL"
                                     << "num INTEGER"
                                     << "title TEXT"
                                     << "lat REAL"
                                     << "lon REAL"
                                     << "hmsl INTEGER"
                                     << "dN INTEGER"
                                     << "dE INTEGER"
                                     << "approach INTEGER"
                                     << "type TEXT"
                                     << "FOREIGN KEY(missionID) REFERENCES Missions(key) ON DELETE "
                                        "CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "Runways", "missionID", false);
    new DBReqMakeIndex(this, "Runways", "lat,lon", false);

    new DBReqMakeTable(this,
                       "Waypoints",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "missionID INTEGER NOT NULL"
                                     << "num INTEGER"
                                     << "title TEXT"
                                     << "lat REAL"
                                     << "lon REAL"
                                     << "altitude INTEGER"
                                     << "type TEXT"
                                     << "actions TEXT"
                                     << "FOREIGN KEY(missionID) REFERENCES Missions(key) ON DELETE "
                                        "CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "Waypoints", "missionID", false);
    new DBReqMakeIndex(this, "Waypoints", "lat,lon", false);

    new DBReqMakeTable(this,
                       "Taxiways",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "missionID INTEGER NOT NULL"
                                     << "num INTEGER"
                                     << "title TEXT"
                                     << "lat REAL"
                                     << "lon REAL"
                                     << "FOREIGN KEY(missionID) REFERENCES Missions(key) ON DELETE "
                                        "CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "Taxiways", "missionID", false);
    new DBReqMakeIndex(this, "Taxiways", "lat,lon", false);

    new DBReqMakeTable(this,
                       "Pois",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "missionID INTEGER NOT NULL"
                                     << "num INTEGER"
                                     << "title TEXT"
                                     << "lat REAL"
                                     << "lon REAL"
                                     << "hmsl INTEGER"
                                     << "radius INTEGER"
                                     << "loops INTEGER"
                                     << "timeout INTEGER"
                                     << "FOREIGN KEY(missionID) REFERENCES Missions(key) ON DELETE "
                                        "CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "Pois", "missionID", false);
    new DBReqMakeIndex(this, "Pois", "lat,lon", false);
}
//=============================================================================
DBReqMissions::DBReqMissions()
    : DatabaseRequest(Database::instance()->missions)
{}
//=============================================================================
//=============================================================================
bool DBReqMissionsUpdateDetails::run(QSqlQuery &query)
{
    if (!missionID) {
        qWarning() << "missing missionID";
        return false;
    }
    //find existing mission details
    query.prepare("SELECT * FROM MissionDetails WHERE missionID=?");
    query.addBindValue(missionID);
    if (!query.exec())
        return false;

    if (query.next()) {
        //mission details exists
        qDebug() << "mission details exists";
        quint64 detailsID = query.value(0).toULongLong();
        //update existing mission detail record with actual data
        if (recordUpdateQuery(query, details, "MissionDetails", "WHERE key=?")) {
            query.addBindValue(detailsID);
            if (!query.exec())
                return false;
            emit dbModified();
        }
        return true;
    }
    //create new details record
    details["missionID"] = missionID;
    if (!recordInsertQuery(query, details, "MissionDetails"))
        return false;
    if (!query.exec())
        return false;
    emit dbModified();
    return true;
}
//=============================================================================
bool DBReqMissionsFindSite::run(QSqlQuery &query)
{
    if (!(std::isnan(lat) || std::isnan(lon) || lat == 0.0 || lon == 0.0)) {
        query.prepare("SELECT (((?-lat)*(?-lat)*?)+((?-lon)*(?-lon))) AS dist, * FROM Sites"
                      " WHERE lat!=0 AND lon!=0 AND dist<2"
                      " ORDER BY dist ASC LIMIT 1");
        query.addBindValue(lat);
        query.addBindValue(lat);
        query.addBindValue(std::pow(std::cos(qDegreesToRadians(lat)), 2));
        query.addBindValue(lon);
        query.addBindValue(lon);
        if (!query.exec())
            return false;
        if (query.next()) {
            siteID = query.value("key").toULongLong();
            site = query.value("title").toString();
        }
    }
    emit siteFound(siteID, site);
    return true;
}
bool DBReqMissionsSaveSite::run(QSqlQuery &query)
{
    if (std::isnan(lat) || std::isnan(lon) || lat == 0.0 || lon == 0.0)
        return false;
    if (!key) {
        //create site record
        query.prepare("INSERT INTO Sites"
                      " (title,lat,lon) VALUES(?,?,?)");
        query.addBindValue(title);
        query.addBindValue(lat);
        query.addBindValue(lon);
        if (!query.exec())
            return false;
        emit siteAdded(title);
    } else {
        //update site record
        query.prepare("UPDATE Sites SET"
                      " title=?,lat=?,lon=?"
                      " WHERE key=?");
        query.addBindValue(title);
        query.addBindValue(lat);
        query.addBindValue(lon);
        query.addBindValue(key);
        if (!query.exec())
            return false;
        emit siteModified(title);
    }
    return true;
}
bool DBReqMissionsRemoveSite::run(QSqlQuery &query)
{
    query.prepare("DELETE FROM Sites WHERE key=?");
    query.addBindValue(key);
    if (!query.exec())
        return false;
    emit siteRemoved();
    return true;
}
//=============================================================================
//=============================================================================
bool DBReqMissionsSave::run(QSqlQuery &query)
{
    if (rw.values.isEmpty()) {
        qWarning() << "missing runways in mission";
    }

    //generate hash
    QCryptographicHash h(QCryptographicHash::Sha1);
    getHash(h, rw);
    getHash(h, wp);
    getHash(h, tw);
    getHash(h, pi);
    QString hash = h.result().toHex().toUpper();
    info["hash"] = hash;
    info["time"] = t;

    //find siteID
    if (!reqSite.run(query))
        return false;
    if (reqSite.siteID) {
        info["siteID"] = reqSite.siteID;
        info["title"]
            = info["title"].toString().remove(reqSite.site, Qt::CaseInsensitive).simplified();
    }
    info = filterNullValues(info);

    //find existing mission by hash
    query.prepare("SELECT * FROM Missions WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        missionID = query.value(0).toULongLong();
        //update mission info
        query.prepare("UPDATE Missions SET time=?, title=?, siteID=? WHERE key=?");
        query.addBindValue(t);
        query.addBindValue(info.value("title"));
        query.addBindValue(info.value("siteID"));
        query.addBindValue(missionID);
        if (!query.exec())
            return false;
        if (!DBReqMissionsUpdateDetails::run(query))
            return false;
        emit missionHash(hash);
        return true;
    }

    //create new mission record
    if (!db->transaction(query))
        return false;

    if (!recordInsertQuery(query, info, "Missions"))
        return false;
    if (!query.exec())
        return false;
    missionID = query.lastInsertId().toULongLong();
    info["key"] = missionID;

    //write items
    rw.names.append("missionID");
    for (int i = 0; i < rw.values.size(); ++i) {
        recordInsertQuery(query, rw, i, "Runways");
        query.addBindValue(missionID);
        if (!query.exec())
            return false;
    }
    wp.names.append("missionID");
    for (int i = 0; i < wp.values.size(); ++i) {
        recordInsertQuery(query, wp, i, "Waypoints");
        query.addBindValue(missionID);
        if (!query.exec())
            return false;
    }
    tw.names.append("missionID");
    for (int i = 0; i < tw.values.size(); ++i) {
        recordInsertQuery(query, tw, i, "Taxiways");
        query.addBindValue(missionID);
        if (!query.exec())
            return false;
    }
    pi.names.append("missionID");
    for (int i = 0; i < pi.values.size(); ++i) {
        recordInsertQuery(query, pi, i, "Pois");
        query.addBindValue(missionID);
        if (!query.exec())
            return false;
    }

    if (!DBReqMissionsUpdateDetails::run(query))
        return false;
    if (!db->commit(query))
        return false;

    emit missionHash(hash);
    return true;
}
void DBReqMissionsSave::makeRecords(const DictMission::Mission &mission)
{
    info["title"] = mission.title;
    info["lat"] = mission.lat;
    info["lon"] = mission.lon;
    //items
    makeRecords(mission.runways, rw);
    makeRecords(mission.waypoints, wp);
    makeRecords(mission.taxiways, tw);
    makeRecords(mission.pois, pi);
}
void DBReqMissionsSave::makeRecords(const QList<DictMission::Item> &items,
                                    DatabaseRequest::Records &records)
{
    records.names << "num"
                  << "title"
                  << "lat"
                  << "lon";
    for (int i = 0; i < items.size(); ++i) {
        const DictMission::Item &f = items.at(i);
        QVariantList r;
        r.append(i);
        r.append(f.title);
        r.append(f.lat);
        r.append(f.lon);
        foreach (QString key, f.details.keys()) {
            if (f.details.value(key).toString().isEmpty())
                continue;
            if (!records.names.contains(key))
                records.names.append(key);
            int j = records.names.indexOf(key);
            while (r.size() <= j)
                r.append(QVariant());
            r[j] = f.details.value(key);
        }
        records.values.append(r);
    }
    //snap records values size
    int sz = records.names.size();
    for (int i = 0; i < records.values.size(); ++i) {
        QVariantList &r = records.values[i];
        while (r.size() < sz)
            r.append(QVariant());
    }
}
//=============================================================================
bool DBReqMissionsLoad::run(QSqlQuery &query)
{
    query.prepare(
        "SELECT * FROM Missions"
        " LEFT JOIN (SELECT key, title AS site FROM Sites) AS Sites  ON Missions.siteID=Sites.key"
        " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    quint64 missionID = query.value("key").toULongLong();
    //mission info
    info = filterIdValues(queryRecord(query));
    mission.title = info.value("title").toString();
    mission.lat = info.value("lat").toDouble();
    mission.lon = info.value("lon").toDouble();

    //mission details
    query.prepare("SELECT * FROM MissionDetails"
                  " WHERE missionID=?");
    query.addBindValue(missionID);
    if (!query.exec())
        return false;
    if (query.next()) {
        details = filterIdValues(queryRecord(query));
    }

    query.prepare("SELECT * FROM Runways WHERE missionID=? ORDER BY num ASC");
    query.addBindValue(missionID);
    if (!query.exec())
        return false;
    readItems(query, mission.runways);

    query.prepare("SELECT * FROM Waypoints WHERE missionID=? ORDER BY num ASC");
    query.addBindValue(missionID);
    if (!query.exec())
        return false;
    readItems(query, mission.waypoints);

    query.prepare("SELECT * FROM Taxiways WHERE missionID=? ORDER BY num ASC");
    query.addBindValue(missionID);
    if (!query.exec())
        return false;
    readItems(query, mission.taxiways);

    query.prepare("SELECT * FROM Pois WHERE missionID=? ORDER BY num ASC");
    query.addBindValue(missionID);
    if (!query.exec())
        return false;
    readItems(query, mission.pois);

    emit loaded(info, details, mission);
    return true;
}
void DBReqMissionsLoad::readItems(QSqlQuery &query, QList<DictMission::Item> &items)
{
    while (query.next()) {
        DictMission::Item e;
        e.title = query.value("title").toString();
        e.lat = query.value("lat").toDouble();
        e.lon = query.value("lon").toDouble();
        e.details = queryRecord(query);
        e.details.remove("key");
        e.details.remove("missionID");
        e.details.remove("num");
        e.details.remove("title");
        e.details.remove("lat");
        e.details.remove("lon");
        items.append(e);
    }
}
//=============================================================================
