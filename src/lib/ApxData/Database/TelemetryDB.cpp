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
#include <App/AppDirs.h>
#include <App/AppLog.h>
//=============================================================================
TelemetryDB::TelemetryDB(QObject *parent, QString sessionName)
    : DatabaseSession(parent, AppDirs::db().absoluteFilePath("telemetry.db"), sessionName)
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
void TelemetryDB::emptyTrash()
{
    if (!checkActive())
        return;
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
    if (!checkActive())
        return;
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
            f_stats->setStatus(
                QString("%1 %2, %3 %4").arg(total).arg(tr("total")).arg(trash).arg(tr("trash")));
        },
        Qt::QueuedConnection);
    req->exec();
}
//=============================================================================
bool TelemetryDB::checkActive()
{
    if (queueSize() > 0 || m_worker->rate() > 0) {
        apxMsgW() << tr("Can't modify database while datalink is active");
        return false;
    }
    return true;
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
bool DBReqTelemetryEmptyTrash::run(QSqlQuery &query)
{
    emit progress(0);
    query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    qint64 cnt = query.value(0).toLongLong();
    qint64 dcnt = 0;
    if (cnt > 0) {
        apxMsg() << tr("Permanently deleting %1 records").arg(cnt).append("...");

        while (dcnt < cnt) {
            query.prepare(
                "SELECT key FROM Telemetry WHERE trash IS NOT NULL ORDER BY time DESC LIMIT 1");
            if (!query.exec())
                return false;
            if (!query.next())
                return false;
            quint64 key = query.value(0).toULongLong();

            if (!db->transaction(query))
                return false;
            query.prepare("DELETE FROM Telemetry WHERE key=?");
            query.addBindValue(key);
            if (!query.exec())
                return false;
            if (!db->commit(query))
                return false;

            dcnt++;
            emit progress((dcnt * 100 / cnt));
            if (discarded())
                break;
        }
    }
    if (dcnt < cnt)
        apxMsgW() << tr("Telemetry trash not empty") << cnt - dcnt;
    else
        apxMsg() << tr("Telemetry trash is empty");
    emit progress(-1);
    return true;
}
//=============================================================================
bool DBReqTelemetryEmptyCache::run(QSqlQuery &query)
{
    emit progress(0);
    query.prepare("SELECT COUNT(*) FROM TelemetryCache");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    qint64 cnt = query.value(0).toLongLong();
    qint64 dcnt = 0;
    if (cnt > 0) {
        apxMsg() << tr("Deleting %1 cached records").arg(cnt).append("...");

        while (dcnt < cnt) {
            query.prepare("SELECT key FROM TelemetryCache ORDER BY time DESC LIMIT 1");
            if (!query.exec())
                return false;
            if (!query.next())
                return false;
            quint64 key = query.value(0).toULongLong();

            if (!db->transaction(query))
                return false;
            query.prepare("DELETE FROM TelemetryCache WHERE key=?");
            query.addBindValue(key);
            if (!query.exec())
                return false;
            if (!db->commit(query))
                return false;

            dcnt++;
            emit progress((dcnt * 100 / cnt));
            if (discarded())
                break;
        }
    }
    if (dcnt < cnt)
        apxMsgW() << tr("Telemetry cache not empty") << cnt - dcnt;
    else
        apxMsg() << tr("Telemetry cache is empty");
    emit progress(-1);
    return true;
}
//=============================================================================
bool DBReqTelemetryStats::run(QSqlQuery &query)
{
    query.prepare("SELECT COUNT(*) FROM Telemetry");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 cntTotal = query.value(0).toULongLong();

    query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 cntTrash = query.value(0).toULongLong();

    apxMsg() << tr("Telemetry records").append(":") << cntTotal << tr("total").append(",")
             << cntTrash << tr("trash");

    emit totals(cntTotal, cntTrash);
    return true;
}
//=============================================================================
