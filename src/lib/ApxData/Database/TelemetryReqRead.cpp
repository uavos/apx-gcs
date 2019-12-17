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
#include "TelemetryReqRead.h"
#include "Database.h"
#define MAX_CACHE_RECORDS 25000000
//=============================================================================
bool DBReqTelemetryFindCache::run(QSqlQuery &query)
{
    //check for invalid cache (telemetry hash is null)
    query.prepare("SELECT * FROM TelemetryStats WHERE telemetryID=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    bool invalid;
    if (query.next())
        invalid = query.value("hash").toString().isEmpty();
    else
        invalid = true;

    //check db invalid cache list
    if (!invalid) {
        if (!static_cast<TelemetryDB *>(db)->invalidCacheList().isEmpty()) {
            invalid = static_cast<TelemetryDB *>(db)->invalidCacheList().contains(telemetryID);
        }
    }

    //find existing cache record
    if (!invalid) {
        query.prepare("SELECT * FROM TelemetryCache WHERE telemetryID=?");
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
        if (query.next()) {
            cacheID = query.value(0).toULongLong();
        }
    }
    if (!cacheID) {
        emit cacheNotFound(telemetryID);
        return true;
    }
    //cache exists
    query.prepare("UPDATE TelemetryCache SET time=?"
                  " WHERE key=?");
    query.addBindValue(t);
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    if (discarded())
        return true;
    emit cacheFound(telemetryID);
    return true;
}
//=============================================================================
bool DBReqTelemetryMakeCache::run(QSqlQuery &query)
{
    QElapsedTimer t0;
    t0.start();

    QList<quint64> rmList;
    //check db invalid cache list
    if (!static_cast<TelemetryDB *>(db)->invalidCacheList().isEmpty()) {
        rmList = static_cast<TelemetryDB *>(db)->invalidCacheList();
        static_cast<TelemetryDB *>(db)->clearInvalidCacheList();
    }
    if (forceUpdate)
        rmList.append(telemetryID);

    for (int i = 0; i < rmList.size(); ++i) {
        quint64 key = rmList.at(i);
        if (!db->transaction(query))
            return false;
        query.prepare("DELETE FROM TelemetryCache WHERE telemetryID=?");
        query.addBindValue(key);
        if (!query.exec())
            return false;
        if (!db->commit(query))
            return false;
        qDebug() << "cache invalidated for" << key;
    }

    if (!DBReqTelemetryFindCache::run(query))
        return false;
    if (cacheID)
        return true; //cache found

    //check cache size and remove some if necessary
    while (1) {
        if (discarded())
            return true;
        query.prepare("SELECT SUM(records) FROM TelemetryCache");
        if (!query.exec())
            return false;
        if (!query.next())
            return false;
        quint64 count = query.value(0).toULongLong();
        if (count < MAX_CACHE_RECORDS)
            break;
        query.prepare("SELECT key,MIN(time) FROM TelemetryCache");
        if (!query.exec())
            return false;
        if (!query.next())
            return false;
        quint64 key = query.value(0).toULongLong();
        query.prepare("DELETE FROM TelemetryCache WHERE key=?");
        query.addBindValue(key);
        if (!query.exec())
            return false;
        qDebug() << "removed" << key;
    }

    //create new cache record
    if (!db->transaction(query))
        return false;
    bool bCommit = false;
    while (1) {
        query.prepare("INSERT INTO TelemetryCache(telemetryID,time) VALUES(?,?)");
        query.addBindValue(telemetryID);
        query.addBindValue(t);
        if (!query.exec())
            return false;
        if (discarded())
            break;
        cacheID = query.lastInsertId().toULongLong();
        //fill data
        query.prepare("DROP TABLE IF EXISTS tmp");
        if (!query.exec())
            return false;
        query.prepare("CREATE TEMPORARY TABLE tmp AS SELECT time,type,name,value,uid FROM ("
                      " SELECT time,CASE WHEN uplink IS NULL THEN 2 ELSE 3 END AS "
                      "type,name,value,uid FROM TelemetryEvents WHERE telemetryID=?"
                      " UNION ALL"
                      " SELECT time,NULL AS type,fieldID AS name,value,NULL AS uid FROM "
                      "TelemetryDownlink WHERE telemetryID=?"
                      " UNION ALL"
                      " SELECT time,1 AS type,fieldID AS name,value,NULL AS uid FROM "
                      "TelemetryUplink WHERE telemetryID=?"
                      ")"
                      " ORDER BY time");
        query.addBindValue(telemetryID);
        query.addBindValue(telemetryID);
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
        if (discarded())
            break;
        query.prepare("INSERT INTO TelemetryCacheData"
                      " (cacheID,time,type,name,value,uid)"
                      " SELECT ? AS cacheID,* FROM tmp WHERE true");
        query.addBindValue(cacheID);
        if (!query.exec())
            return false;
        if (discarded())
            break;
        query.prepare("DROP TABLE IF EXISTS tmp");
        if (!query.exec())
            return false;
        if (discarded())
            break;

        //update records count
        query.prepare("UPDATE TelemetryCache SET records="
                      " (SELECT COUNT(*) FROM TelemetryCacheData WHERE cacheID=?)"
                      " WHERE key=?");
        query.addBindValue(cacheID);
        query.addBindValue(cacheID);
        if (!query.exec())
            return false;

        bCommit = true;
        break;
    }
    if (bCommit) {
        if (!db->commit(query))
            return false;
        if (discarded())
            return true;
        newCacheID = cacheID;
        qDebug() << t0.elapsed() << "ms";
    } else {
        db->rollback(query);
        cacheID = 0;
    }
    return true;
}
//=============================================================================
bool DBReqTelemetryMakeStats::run(QSqlQuery &query)
{
    QElapsedTimer t0;
    t0.start();
    query.prepare("SELECT * FROM TelemetryStats WHERE telemetryID=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    if (query.next()) {
        //check db invalid cache list
        stats = queryRecord(query);
        if (static_cast<TelemetryDB *>(db)->invalidCacheList().contains(telemetryID)) {
            stats.remove("hash");
        }
    }
    if (discarded())
        return true;
    if (!stats.value("hash").toString().isEmpty()) {
        //stats exists and valid
        if (!DBReqTelemetryMakeCache::run(query))
            return false;
        emit statsFound(telemetryID, stats);
        return true;
    }

    //collect data and update stats record
    if (discarded())
        return true;

    //create cache
    forceUpdate = true;
    if (!DBReqTelemetryMakeCache::run(query))
        return false;
    if (!cacheID) {
        qWarning() << "missing cache";
        return true;
    }

    //data hash
    query.prepare("SELECT time,type,name,value,uid FROM TelemetryCacheData"
                  " WHERE cacheID=?"
                  " ORDER BY time,type,name");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    QCryptographicHash h(QCryptographicHash::Sha1);
    getHash(h, query);
    QString hash = h.result().toHex().toUpper();

    //downlink cnt
    query.prepare("SELECT COUNT(*) FROM TelemetryCacheData"
                  " WHERE type IS NULL AND cacheID=?");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    quint64 downlink = query.next() ? query.value(0).toULongLong() : 0;
    if (discarded())
        return true;

    //uplink cnt
    query.prepare("SELECT COUNT(*) FROM TelemetryCacheData"
                  " WHERE type=1 AND cacheID=?");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    quint64 uplink = query.next() ? query.value(0).toULongLong() : 0;
    if (discarded())
        return true;

    //events cnt
    quint64 events = 0;
    QString evtDetails;
    QMap<QString, quint64> evtCount;
    query.prepare("SELECT COUNT(*),name FROM TelemetryCacheData"
                  " WHERE (type=2 OR type=3) AND cacheID=?"
                  " GROUP BY name ORDER BY name");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    while (query.next()) {
        if (discarded())
            return true;
        quint64 ecnt = query.value(0).toULongLong();
        events += ecnt;
        if (query.value(0).toULongLong())
            evtCount[query.value(1).toString()] = ecnt;
    }
    QStringList st;
    foreach (QString s, evtCount.keys()) {
        st.append(QString("%1=%2").arg(s).arg(evtCount.value(s)));
    }
    evtDetails = st.join(',');
    if (discarded())
        return true;

    //total time in record
    /*query.prepare("SELECT MIN(time),MAX(time) FROM TelemetryCacheData"
                " WHERE cacheID=?"
                );
  query.addBindValue(cacheID);
  if(!query.exec())return false;
  if(!query.next())return false;
*/
    query.prepare("SELECT MIN(time) FROM TelemetryCacheData"
                  " WHERE cacheID=?");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 st0 = query.value(0).toULongLong();
    query.prepare("SELECT MAX(time) FROM TelemetryCacheData"
                  " WHERE cacheID=?");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    quint64 totalTime = query.value(0).toULongLong() - st0;

    //save recalculated values to table
    bool ok = false;
    while (1) {
        if (totalTime != stats.value("totalTime").toULongLong())
            break;
        if (downlink != stats.value("downlink").toULongLong())
            break;
        if (uplink != stats.value("uplink").toULongLong())
            break;
        if (events != stats.value("events").toULongLong())
            break;
        if (evtDetails != stats.value("evtDetails").toString())
            break;
        if (hash != stats.value("hash").toString())
            break;
        ok = true;
        break;
    }
    if (!ok) {
        bool bNew = stats.isEmpty();
        stats["totalTime"] = totalTime;
        stats["downlink"] = downlink;
        stats["uplink"] = uplink;
        stats["events"] = events;
        stats["evtDetails"] = evtDetails;
        stats["hash"] = hash;
        if (bNew) {
            stats["telemetryID"] = telemetryID;
            if (recordInsertQuery(query, stats, "TelemetryStats")) {
                if (!query.exec())
                    return false;
                emit dbModified();
            }
        } else {
            quint64 key = stats.value("key").toULongLong();
            stats.remove("key");
            if (recordUpdateQuery(query, stats, "TelemetryStats", "WHERE key=?")) {
                query.addBindValue(key);
                if (!query.exec())
                    return false;
                emit dbModified();
            }
        }
    }

    if (discarded())
        return true;
    stats.remove("key");
    stats.remove("telemetryID");

    qDebug() << t0.elapsed() << "ms";
    emit statsUpdated(telemetryID, stats);
    return true;
}
//=============================================================================
bool DBReqTelemetryReadData::run(QSqlQuery &query)
{
    //QTime t0;
    //t0.start();
    if (discarded())
        return true;
    emit progress(telemetryID, 0);

    //get count for progress
    query.prepare("SELECT * FROM TelemetryStats"
                  " WHERE telemetryID=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    quint64 count = 0;
    if (query.next()) {
        count += query.value("downlink").toULongLong();
        count += query.value("uplink").toULongLong();
        count += query.value("events").toULongLong();
        stats = filterIdValues(queryRecord(query));
    }
    //qDebug()<<t0.elapsed()<<"ms"<<count;

    //info
    query.prepare("SELECT * FROM Telemetry"
                  " WHERE key=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    info = filterIdValues(queryRecord(query));

    //data
    query.prepare("SELECT * FROM TelemetryCache"
                  " WHERE telemetryID=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;
    cacheID = query.value(0).toULongLong();

    query.prepare("SELECT time,type,name,value,uid FROM TelemetryCacheData"
                  " WHERE cacheID=?");
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;

    quint64 row = 0;
    int vp_s = 0;
    records.names << "time"
                  << "type"
                  << "name"
                  << "value"
                  << "uid";
    //qDebug()<<t0.elapsed()<<"ms";
    while (query.next()) {
        if (discarded())
            return true;
        int vp = count > 0 ? row * 100 / count : 0;
        if (vp_s != vp) {
            vp_s = vp;
            emit progress(telemetryID, vp);
        }
        row++;
        records.values.append(QVariantList() << query.value(0) << query.value(1) << query.value(2)
                                             << query.value(3) << query.value(4));
    }
    if (discarded())
        return true;

    //collect fields map
    query.prepare("SELECT key, name FROM TelemetryFields");
    if (!query.exec())
        return false;
    while (query.next()) {
        quint64 fid = query.value(0).toULongLong();
        fieldNames.insert(fid, query.value(1).toString());
    }

    emit dataLoaded(telemetryID, cacheID, records, fieldNames);
    emit progress(telemetryID, -1);

    return true;
}
//=============================================================================
//=============================================================================
bool DBReqTelemetryReadEvents::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM TelemetryCacheData"
                  " WHERE time>=? AND (type=2 OR type=3 OR type=1) AND cacheID=?");
    query.addBindValue(time);
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    Records v = queryRecords(query);
    //if(v.values.isEmpty())return true;
    //qDebug()<<time<<tMax<<records.values.size();
    emit eventsLoaded(v);
    return true;
}
//=============================================================================
bool DBReqTelemetryReadEvent::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM TelemetryCacheData"
                  " WHERE time<=? AND (type=2 OR type=3) AND name=? AND cacheID=?"
                  " ORDER BY time DESC LIMIT 1");
    query.addBindValue(time);
    query.addBindValue(name);
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    if (!query.next()) {
        //find any data
        query.prepare("SELECT * FROM TelemetryCacheData"
                      " WHERE (type=2 OR type=3) AND name=? AND cacheID=?"
                      " LIMIT 1");
        query.addBindValue(name);
        query.addBindValue(cacheID);
        if (!query.exec())
            return false;
        if (!query.next())
            return true;
    }
    emit eventLoaded(query.value("value").toString(), query.value("uid").toString(), false);
    return true;
}
//=============================================================================
bool DBReqTelemetryReadConfData::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM TelemetryCacheData"
                  " WHERE time<=? AND (type=2 OR type=3) AND name='conf' AND cacheID=?");
    query.addBindValue(time);
    query.addBindValue(cacheID);
    if (!query.exec())
        return false;
    Records v = queryRecords(query);
    if (v.values.isEmpty())
        return true;
    emit confLoaded(v);
    return true;
}
//=============================================================================
//=============================================================================
bool DBReqTelemetryReadSharedHashId::run(QSqlQuery &query)
{
    if (hash.isEmpty()) {
        query.prepare("SELECT * FROM TelemetryShare"
                      " INNER JOIN Telemetry ON TelemetryShare.telemetryID=Telemetry.key"
                      " WHERE telemetryID=? AND trash IS NULL");
        query.addBindValue(telemetryID);
        if (!query.exec())
            return false;
        if (!query.next())
            return true;
        hash = query.value("sharedHash").toString();
        emit foundHash(hash);
        return true;
    }
    query.prepare("SELECT * FROM TelemetryShare"
                  " INNER JOIN Telemetry ON TelemetryShare.telemetryID=Telemetry.key"
                  " WHERE sharedHash=? AND trash IS NULL"
                  " LIMIT 1");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (!query.next())
        return true;
    telemetryID = query.value(0).toULongLong();
    emit foundID(telemetryID);
    return true;
}
//=============================================================================
bool DBReqTelemetryFindDataHash::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM TelemetryStats"
                  " INNER JOIN Telemetry ON TelemetryStats.telemetryID=Telemetry.key"
                  " WHERE hash=? AND trash IS NULL");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (!query.next())
        return true;
    telemetryID = query.value("telemetryID").toULongLong();
    emit foundID(telemetryID);
    return true;
}
//=============================================================================
bool DBReqTelemetryDeleteRecord::run(QSqlQuery &query)
{
    query.prepare("DELETE FROM Telemetry WHERE key=?");
    query.addBindValue(telemetryID);
    if (!query.exec())
        return false;
    emit deletedID(telemetryID);
    return true;
}
//=============================================================================
