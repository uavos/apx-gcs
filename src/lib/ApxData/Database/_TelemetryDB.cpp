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
#include "Database.h"
#include "TelemetryDB.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>

// TODO move cache tables to external file

TelemetryDB::TelemetryDB(QObject *parent, QString sessionName)
    : DatabaseSession(parent, "telemetry", sessionName, "1")
    , latestInvalidCacheID(0)
{
    qRegisterMetaType<QMap<quint64, QString>>("QMap<quint64,QString>");

    f_stats = new Fact(this, "stats", tr("Get statistics"), tr("Analyze totals"), NoFlags, "numeric");
    connect(f_stats, &Fact::triggered, this, &TelemetryDB::getStats);
    connect(this, &Fact::triggered, this, &TelemetryDB::getStats);

    f_cache = new Fact(this,
                       "cache",
                       tr("Reset cache"),
                       tr("Invalidate cached records"),
                       NoFlags,
                       "cached");
    connect(f_cache, &Fact::triggered, this, &TelemetryDB::emptyCache);
    connect(f_cache, &Fact::progressChanged, this, [this]() { setProgress(f_cache->progress()); });

    f_trash = new Fact(this,
                       "trash",
                       tr("Empty trash"),
                       tr("Permanently remove deleted records"),
                       Remove);
    connect(f_trash, &Fact::triggered, this, &TelemetryDB::emptyTrash);
    connect(f_trash, &Fact::progressChanged, this, [this]() { setProgress(f_trash->progress()); });

    f_stop = new Fact(this, "stop", tr("Abort"), tr("Abort current operation"), Action | Stop);
    f_stop->setEnabled(false);
    connect(this, &Fact::progressChanged, this, [this]() { f_stop->setEnabled(progress() >= 0); });

    new DBReqMakeTable(this,
                       "Telemetry",
                       QStringList() << "key INTEGER PRIMARY KEY NOT NULL"
                                     << "trash INTEGER"
                                     << "time INTEGER" //[ms since epoch]
                                     << "vehicleUID TEXT"
                                     << "callsign TEXT"
                                     << "notes TEXT"
                                     << "comment TEXT"
                                     << "hash TEXT"
                                     //  << "downlink INTEGER"
                                     //  << "uplink INTEGER"
                                     //  << "events INTEGER"
                                     //  << "evtDetails TEXT"
                                     // file
                                     << "parsed INTEGER"
                                     << "duration INTEGER"
                                     << "file TEXT"
                                     << "info TEXT");

    new DBReqMakeIndex(this, "Telemetry", "trash", false);
    new DBReqMakeIndex(this, "Telemetry", "time", false);
    new DBReqMakeIndex(this, "Telemetry", "vehicleUID", false);
    new DBReqMakeIndex(this, "Telemetry", "callsign", false);
    new DBReqMakeIndex(this, "Telemetry", "file", true);

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
                                     << "name TEXT NOT NULL"
                                     << "title TEXT"
                                     << "units TEXT");
    new DBReqMakeIndex(this, "TelemetryFields", "id", false);
    new DBReqMakeIndex(this, "TelemetryFields", "name", true);

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
TelemetryDB::~TelemetryDB()
{
    //Database::instance()->telemetry = nullptr;
}

void TelemetryDB::updateFieldsMap(FieldsByUID byUID, FieldsByName byName)
{
    QMutexLocker lock(&pMutex);
    _fieldsByUID = byUID;
    _fieldsByName = byName;

    for (auto s : byName.keys())
        _uidByName.insert(s, byUID.key(byName.value(s)));
}
quint64 TelemetryDB::field_key(mandala::uid_t uid)
{
    QMutexLocker lock(&pMutex);
    auto key = _fieldsByUID.value(uid);
    if (!key) {
        qWarning() << "missing field uid" << uid;
    }
    return key;
}
quint64 TelemetryDB::field_key(QString name)
{
    QMutexLocker lock(&pMutex);
    auto key = _fieldsByName.value(name);
    if (!key) {
        qWarning() << "missing field name" << name;
    }
    return key;
}
mandala::uid_t TelemetryDB::mandala_uid(QString name)
{
    // used by file import
    QMutexLocker lock(&pMutex);
    return _uidByName.value(name);
}
mandala::uid_t TelemetryDB::mandala_uid(quint64 field_key)
{
    // used by telemetry reader
    QMutexLocker lock(&pMutex);
    return _fieldsByUID.key(field_key);
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
    auto m = m_invalidCacheList;
    m.detach();
    return m;
}
void TelemetryDB::clearInvalidCacheList()
{
    QMutexLocker lock(&pMutex);
    m_invalidCacheList.clear();
    latestInvalidCacheID = 0;
}

void TelemetryDB::emptyTrash()
{
    DBReqTelemetryEmptyTrash *req = new DBReqTelemetryEmptyTrash();
    connect(
        req,
        &DBReqTelemetryEmptyTrash::progress,
        this,
        [this](int v) { f_trash->setProgress(v); },
        Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryEmptyTrash::finished,
            this,
            &TelemetryDB::invalidateRecords,
            Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryEmptyTrash::finished,
            this,
            &TelemetryDB::getStats,
            Qt::QueuedConnection);
    connect(f_stop, &Fact::triggered, req, &DatabaseRequest::discard, Qt::QueuedConnection);
    req->exec();
}
void TelemetryDB::emptyCache()
{
    DBReqTelemetryEmptyCache *req = new DBReqTelemetryEmptyCache();
    connect(
        req,
        &DBReqTelemetryEmptyCache::progress,
        this,
        [this](int v) { f_cache->setProgress(v); },
        Qt::QueuedConnection);
    connect(f_stop, &Fact::triggered, req, &DatabaseRequest::discard, Qt::QueuedConnection);
    req->exec();
}
void TelemetryDB::getStats()
{
    DBReqTelemetryStats *req = new DBReqTelemetryStats();
    connect(
        req,
        &DBReqTelemetryStats::totals,
        this,
        [this](quint64 total, quint64 trash) {
            f_stats->setValue(
                QString("%1 %2, %3 %4").arg(total).arg(tr("total")).arg(trash).arg(tr("trash")));
        },
        Qt::QueuedConnection);
    req->exec();
}
