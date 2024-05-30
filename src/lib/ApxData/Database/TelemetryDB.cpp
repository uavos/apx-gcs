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
#include "TelemetryDB.h"
#include "Database.h"
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
                                     << "totalTime INTEGER"
                                     << "downlink INTEGER"
                                     << "uplink INTEGER"
                                     << "events INTEGER"
                                     << "evtDetails TEXT");
    new DBReqMakeIndex(this, "Telemetry", "trash", false);
    new DBReqMakeIndex(this, "Telemetry", "time", false);
    new DBReqMakeIndex(this, "Telemetry", "vehicleUID", false);
    new DBReqMakeIndex(this, "Telemetry", "callsign", false);
    new DBReqMakeIndex(this, "Telemetry", "hash", false);

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
    Database::instance()->telemetry = nullptr;
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

DBReqTelemetry::DBReqTelemetry()
    : DatabaseRequest(Database::instance()->telemetry)
{}
bool DBReqTelemetry::run(QSqlQuery &query)
{
    Q_UNUSED(query)
    return true;
}

bool DBReqTelemetryUpdateMandala::run(QSqlQuery &query)
{
    // called by LOCAL vehicle with 'records' initialized to current mandala fields
    connect(
        this,
        &DBReqTelemetryUpdateMandala::progress,
        db,
        [this](int v) { db->setProgress(v); },
        Qt::QueuedConnection);

    const QStringList &n = records.names;
    int i_id = n.indexOf("id");
    int i_name = n.indexOf("name");

    TelemetryDB::FieldsByUID byUID;
    TelemetryDB::FieldsByName byName;
    QList<quint64> all_keys;

    //load existing fields
    query.prepare("SELECT * FROM TelemetryFields");
    if (!query.exec())
        return false;
    Records db_records = queryRecords(query);
    const QStringList &rn = db_records.names;
    int i_r_name = rn.indexOf("name");

    bool mod = false;
    for (auto const &f : records.values) {
        QString f_name = f.at(i_name).toString();
        mandala::uid_t uid = f.at(i_id).toUInt();

        quint64 key = 0;
        bool upd = false;
        for (auto const &r : db_records.values) {
            const QString &name = r.at(i_r_name).toString();
            if (name == f_name) {
                key = r.at(0).toULongLong();
                byUID.insert(uid, key);
                byName.insert(f_name, key);
                all_keys.append(key);
                //check entire row match
                for (int i = 1; i < r.size(); ++i) {
                    if (r.at(i).toString() == f.value(n.indexOf(rn.at(i))).toString()) {
                        continue;
                    }
                    //qWarning() << r.at(i) << f.value(n.indexOf(rn.at(i)));
                    upd = true; //record update needed
                    break;
                }
                break; //exsisting record found
            }
        }
        if (key && !upd)
            continue;
        if (!mod) {
            if (!db->transaction(query))
                return false;
            mod = true;
        }
        if (!key) {
            apxConsole() << "new telemetry field:" << f_name;
            query.prepare("INSERT INTO TelemetryFields(name) VALUES(?)");
            query.addBindValue(f_name);
            if (!query.exec())
                return false;
            key = query.lastInsertId().toULongLong();
            byUID.insert(uid, key);
            byName.insert(f_name, key);
            all_keys.append(key);
        } else {
            apxConsole() << "update telemetry field:" << f_name;
        }
        //update existing record
        QStringList nlist = rn;
        if (!nlist.isEmpty()) {
            nlist.removeAt(0); //skip key
        }
        if (nlist.isEmpty()) {
            nlist = n;
            nlist.removeOne("alias");
        }
        QString qs = QString("UPDATE TelemetryFields SET %1=? WHERE key=?").arg(nlist.join("=?,"));
        query.prepare(qs);
        for (auto s : nlist) {
            query.addBindValue(f.value(n.indexOf(s)));
        }
        query.addBindValue(key);
        if (!query.exec())
            return false;
    }
    if (mod) {
        if (!db->commit(query))
            return false;
        apxMsgW() << tr("Telemetry DB updated");
    }

    //check for deprecated records
    QList<quint64> rmlist;
    for (auto const &r : db_records.values) {
        quint64 key = r.at(0).toULongLong();
        if (all_keys.contains(key))
            continue;
        apxConsole() << "remove telemetry field:" << r.at(i_r_name).toString();
        rmlist.append(key);
    }
    if (!rmlist.isEmpty()) {
        db->disable();
        emit progress(0);
        apxMsgW() << tr("Telemetry DB maintenance in progress...");
        QStringList st;
        for (auto k : rmlist)
            st << QString::number(k);
        if (!query.exec("PRAGMA foreign_keys = OFF"))
            return false;
        query.prepare(
            QString("DELETE FROM TelemetryUplink WHERE fieldID IN (%1)").arg(st.join(',')));
        if (!query.exec())
            return false;
        query.prepare(
            QString("DELETE FROM TelemetryDownlink WHERE fieldID IN (%1)").arg(st.join(',')));
        if (!query.exec())
            return false;
        query.prepare(QString("DELETE FROM TelemetryFields WHERE key IN (%1)").arg(st.join(',')));
        if (!query.exec())
            return false;
        if (!query.exec("PRAGMA foreign_keys = ON"))
            return false;
        emit progress(-1);
        apxMsgW() << tr("Telemetry DB ready");
        db->enable();
    }

    static_cast<TelemetryDB *>(db)->updateFieldsMap(byUID, byName);

    return true;
}

bool DBReqTelemetryEmptyTrash::run(QSqlQuery &query)
{
    db->disable();
    emit progress(0);
    bool rv = false;
    do {
        query.prepare("SELECT COUNT(*) FROM Telemetry WHERE trash IS NOT NULL");
        if (!query.exec())
            break;
        if (!query.next())
            break;

        qint64 cnt = query.value(0).toLongLong();
        qint64 dcnt = 0;
        if (cnt > 0) {
            apxMsg() << tr("Permanently deleting %1 records").arg(cnt).append("...");

            while (dcnt < cnt) {
                query.prepare(
                    "SELECT key FROM Telemetry WHERE trash IS NOT NULL ORDER BY time DESC LIMIT 1");
                if (!query.exec())
                    break;
                if (!query.next())
                    break;
                quint64 key = query.value(0).toULongLong();

                if (!db->transaction(query))
                    break;
                query.prepare("DELETE FROM Telemetry WHERE key=?");
                query.addBindValue(key);
                if (!query.exec())
                    break;
                if (!db->commit(query))
                    break;

                dcnt++;
                emit progress((dcnt * 100 / cnt));
                if (discarded())
                    break;
            }
        }
        if (dcnt < cnt)
            apxMsgW() << tr("Telemetry trash not empty") << cnt - dcnt;
        else {
            apxMsg() << tr("Telemetry trash is empty");
            rv = true;
        }
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
}

bool DBReqTelemetryEmptyCache::run(QSqlQuery &query)
{
    db->disable();
    emit progress(0);
    bool rv = false;
    do {
        query.prepare("SELECT COUNT(*) FROM TelemetryCache");
        if (!query.exec())
            break;
        if (!query.next())
            break;

        qint64 cnt = query.value(0).toLongLong();
        qint64 dcnt = 0;
        if (cnt > 0) {
            apxMsg() << tr("Deleting %1 cached records").arg(cnt).append("...");

            while (dcnt < cnt) {
                query.prepare("SELECT key FROM TelemetryCache ORDER BY time DESC LIMIT 1");
                if (!query.exec())
                    break;
                if (!query.next())
                    break;
                quint64 key = query.value(0).toULongLong();

                if (!db->transaction(query))
                    break;
                query.prepare("DELETE FROM TelemetryCache WHERE key=?");
                query.addBindValue(key);
                if (!query.exec())
                    break;
                if (!db->commit(query))
                    break;

                dcnt++;
                emit progress((dcnt * 100 / cnt));
                if (discarded())
                    break;
            }
        }
        if (dcnt < cnt)
            apxMsgW() << tr("Telemetry cache not empty") << cnt - dcnt;
        else {
            apxMsg() << tr("Telemetry cache is empty");
            rv = true;
        }
    } while (0);
    emit progress(-1);
    db->enable();
    return rv;
}

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

bool DBReqTelemetryRecover::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Telemetry WHERE hash=?");
    query.addBindValue(_hash);
    if (!query.exec())
        return false;
    if (!query.next()) {
        emit unavailable(_hash);
        return true;
    }
    auto telemetryID = query.value(0).toULongLong();
    auto trash = query.value("trash").toBool();
    if (trash) {
        query.prepare("UPDATE Telemetry SET trash=NULL WHERE key=?");
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
    }
    emit available(telemetryID, _hash);
    return true;
}
