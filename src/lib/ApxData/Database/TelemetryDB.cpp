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
#include "TelemetryDB.h"
#include "Database.h"
#include <ApxDirs.h>
#include <ApxLog.h>
//=============================================================================
TelemetryDB::TelemetryDB(QObject *parent, QString sessionName)
    : DatabaseSession(parent, ApxDirs::db().absoluteFilePath("telemetry.db"), sessionName)
    , latestInvalidCacheID(0)
{
    qRegisterMetaType<QMap<quint64, QString>>("QMap<quint64,QString>");

    new DBReqMakeTable(this,
                       "Telemetry",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "trash INTEGER"
                                     << "time INTEGER" //[ms since epoch]
                                     << "vehicleUID TEXT"
                                     << "callsign TEXT"
                                     << "notes TEXT"
                                     << "comment TEXT");
    new DBReqMakeIndex(this, "Telemetry", "trash", false);
    new DBReqMakeIndex(this, "Telemetry", "time", false);
    new DBReqMakeIndex(this, "Telemetry", "vehicleUID", false);
    new DBReqMakeIndex(this, "Telemetry", "callsign", false);

    new DBReqMakeTable(this,
                       "TelemetryStats",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "telemetryID INTEGER NOT NULL UNIQUE"
                                     << "hash TEXT"
                                     << "totalTime INTEGER"
                                     << "downlink INTEGER"
                                     << "uplink INTEGER"
                                     << "events INTEGER"
                                     << "evtDetails TEXT"
                                     << "FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryStats", "telemetryID", false);
    new DBReqMakeIndex(this, "TelemetryStats", "hash", false);

    new DBReqMakeTable(this,
                       "TelemetryShare",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "telemetryID INTEGER NOT NULL UNIQUE"
                                     << "sharedHash TEXT" //xml file sha1 to skip duplicates
                                     << "machineUID TEXT"
                                     << "hostname TEXT"
                                     << "username TEXT"
                                     << "FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryShare", "telemetryID", false);
    new DBReqMakeIndex(this, "TelemetryShare", "sharedHash", false);

    new DBReqMakeTable(this,
                       "TelemetryFields",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "id INTEGER"
                                     << "name TEXT"
                                     << "title TEXT"
                                     << "descr TEXT"
                                     << "units TEXT"
                                     << "opts TEXT"
                                     << "sect TEXT");

    //---------------------------------
    // data
    new DBReqMakeTable(this,
                       "TelemetryDownlink",
                       QStringList() << "telemetryID INTEGER NOT NULL"
                                     << "time INTEGER" //[ms since file]
                                     << "fieldID INTEGER NOT NULL"
                                     << "value REAL"
                                     << "FOREIGN KEY(fieldID) REFERENCES TelemetryFields(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE"
                                     << "FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryDownlink", "telemetryID", false);

    new DBReqMakeTable(this,
                       "TelemetryUplink",
                       QStringList() << "telemetryID INTEGER NOT NULL"
                                     << "time INTEGER" //[ms since file]
                                     << "fieldID INTEGER NOT NULL"
                                     << "value REAL"
                                     << "FOREIGN KEY(fieldID) REFERENCES TelemetryFields(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE"
                                     << "FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryUplink", "telemetryID", false);

    new DBReqMakeTable(this,
                       "TelemetryEvents",
                       QStringList() << "telemetryID INTEGER NOT NULL"
                                     << "time INTEGER" //[ms since file]
                                     << "name TEXT"
                                     << "value TEXT"
                                     << "uid TEXT"
                                     << "uplink INTEGER"
                                     << "FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryEvents", "telemetryID", false);

    //---------------------------------
    // cache
    new DBReqMakeTable(this,
                       "TelemetryCache",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "telemetryID INTEGER NOT NULL UNIQUE"
                                     << "time INTEGER"    //access time
                                     << "records INTEGER" //count
                                     << "FOREIGN KEY(telemetryID) REFERENCES Telemetry(key) ON "
                                        "DELETE CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryCache", "telemetryID", false);
    new DBReqMakeIndex(this, "TelemetryCache", "time", false);
    new DBReqMakeIndex(this, "TelemetryCache", "records", false);

    new DBReqMakeTable(this,
                       "TelemetryCacheData",
                       QStringList()
                           << "key INTEGER PRIMARY KEY"
                           << "cacheID INTEGER NOT NULL"
                           << "time INTEGER"
                           << "type INTEGER" //0=downlink, 1=uplink, 2=eventDownlink, 3=eventUplink
                           << "name INTEGER"
                           << "value INTEGER"
                           << "uid TEXT"
                           << "FOREIGN KEY(cacheID) REFERENCES TelemetryCache(key) ON DELETE "
                              "CASCADE ON UPDATE CASCADE");
    new DBReqMakeIndex(this, "TelemetryCacheData", "cacheID", false);
    new DBReqMakeIndex(this, "TelemetryCacheData", "time", false);
    new DBReqMakeIndex(this, "TelemetryCacheData", "type", false);
}
//=============================================================================
TelemetryDB::TelemetryFieldsMap TelemetryDB::fieldsMap()
{
    QMutexLocker lock(&pMutex);
    return m_fieldsMap;
}
void TelemetryDB::setFieldsMap(const TelemetryFieldsMap &v)
{
    QMutexLocker lock(&pMutex);
    m_fieldsMap = v;
}
void TelemetryDB::markCacheInvalid(quint64 telemetryID)
{
    if (latestInvalidCacheID == telemetryID)
        return;
    latestInvalidCacheID = telemetryID; //optimize
    QMutexLocker lock(&pMutex);
    if (m_invalidCacheList.contains(telemetryID))
        return;
    m_invalidCacheList.append(telemetryID);
}
QList<quint64> TelemetryDB::invalidCacheList()
{
    QMutexLocker lock(&pMutex);
    return m_invalidCacheList;
}
void TelemetryDB::clearInvalidCacheList()
{
    QMutexLocker lock(&pMutex);
    m_invalidCacheList.clear();
    latestInvalidCacheID = 0;
}
//=============================================================================
//=============================================================================
DBReqTelemetry::DBReqTelemetry()
    : DatabaseRequest(Database::instance()->telemetry)
{}
bool DBReqTelemetry::run(QSqlQuery &query)
{
    Q_UNUSED(query)
    return true;
}
//=============================================================================
//=============================================================================
bool DBReqTelemetryUpdateFields::run(QSqlQuery &query)
{
    const QStringList &n = records.names;
    QStringList fnames;
    for (int i = 0; i < records.values.size(); ++i) {
        const QVariantList &r = records.values.at(i);
        QString s = r.at(n.indexOf("name")).toString();
        fnames.append(s);
    }
    TelemetryDB::TelemetryFieldsMap fmap;
    //check existing fields
    query.prepare("SELECT * FROM TelemetryFields");
    if (!query.exec())
        return false;
    while (query.next()) {
        QString s = query.value(2).toString();
        if (!fnames.contains(s)) {
            apxConsoleW() << "deprecated field telemetry DB" << s;
            continue;
        }
        fnames.removeOne(s);
        fmap.insert(query.value(0).toULongLong(), s);
    }
    //insert missing
    foreach (QString s, fnames) {
        query.prepare("INSERT INTO TelemetryFields(name) VALUES(?)");
        query.addBindValue(s);
        if (!query.exec())
            return false;
        fmap.insert(query.lastInsertId().toULongLong(), s);
        apxConsole() << "add field telemetry DB" << s;
    }
    //update info
    for (int i = 0; i < records.values.size(); ++i) {
        const QVariantList &r = records.values.at(i);
        QString s = r.at(n.indexOf("name")).toString();
        quint64 fieldID = fmap.key(s);
        query.prepare("UPDATE TelemetryFields"
                      " SET name=?, title=?, descr=?, units=?, opts=?, sect=?, id=?"
                      " WHERE key=?");
        query.addBindValue(s);
        query.addBindValue(r.at(n.indexOf("title")));
        query.addBindValue(r.at(n.indexOf("descr")));
        query.addBindValue(r.at(n.indexOf("units")));
        query.addBindValue(r.at(n.indexOf("opts")));
        query.addBindValue(r.at(n.indexOf("sect")));
        query.addBindValue(r.at(n.indexOf("id")));
        query.addBindValue(fieldID);
        if (!query.exec())
            return false;
    }
    static_cast<TelemetryDB *>(db)->setFieldsMap(fmap);
    return true;
}
//=============================================================================
