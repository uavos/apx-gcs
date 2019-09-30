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
#include "LookupTelemetry.h"
#include <Database/Database.h>
#include <Database/TelemetryDB.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>

#include <App/AppRoot.h>
//=============================================================================
LookupTelemetry::LookupTelemetry(Fact *parent)
    : DatabaseLookup(parent,
                     "lookup",
                     tr("Records"),
                     tr("Database lookup"),
                     Database::instance()->telemetry)
    , _findNumId(0)
    , m_recordsCount(0)
    , m_recordNum(0)
    , m_recordId(0)
    , m_recordTimestamp(0)
{
    setOpt("pos", QPointF(1, 1));

    connect(this, &DatabaseLookup::itemTriggered, this, &LookupTelemetry::loadItem);

    //actions
    f_latest = new Fact(this,
                        "latest",
                        tr("Latest"),
                        tr("Load latest"),
                        Action | ShowDisabled | Apply | IconOnly,
                        "fast-forward");
    connect(f_latest, &Fact::triggered, this, &LookupTelemetry::dbLoadLatest);

    f_prev = new Fact(this,
                      "prev",
                      tr("Prev"),
                      tr("Load previous"),
                      Action | ShowDisabled | IconOnly,
                      "chevron-left");
    connect(f_prev, &Fact::triggered, this, &LookupTelemetry::dbLoadPrev);

    f_next = new Fact(this,
                      "next",
                      tr("Next"),
                      tr("Load next"),
                      Action | ShowDisabled | IconOnly,
                      "chevron-right");
    connect(f_next, &Fact::triggered, this, &LookupTelemetry::dbLoadNext);

    f_remove = new Fact(this,
                        "remove",
                        tr("Remove"),
                        tr("Remove current record"),
                        Action | ShowDisabled | Remove | IconOnly,
                        "delete");
    connect(f_remove, &Fact::triggered, this, &LookupTelemetry::dbRemove);

    //status
    connect(this, &LookupTelemetry::recordsCountChanged, this, &LookupTelemetry::updateStatus);
    connect(this, &LookupTelemetry::recordNumChanged, this, &LookupTelemetry::updateStatus);

    //active record highlight
    connect(this,
            &LookupTelemetry::recordIdChanged,
            this,
            &LookupTelemetry::reloadQueryResults,
            Qt::QueuedConnection);
    //connect(this,&LookupTelemetry::recordsCountChanged,this,&LookupTelemetry::dbLoadLatest);

    connect(this, &LookupTelemetry::recordIdChanged, this, &LookupTelemetry::dbLoadInfo);
    connect(this, &LookupTelemetry::recordIdChanged, this, &LookupTelemetry::dbFindNum);
    connect(this, &LookupTelemetry::recordsCountChanged, this, &LookupTelemetry::dbFindNum);

    //actions update
    connect(this, &LookupTelemetry::recordIdChanged, this, &LookupTelemetry::updateActions);
    connect(this, &LookupTelemetry::recordNumChanged, this, &LookupTelemetry::updateActions);
    connect(this, &LookupTelemetry::recordsCountChanged, this, &LookupTelemetry::updateActions);
    updateActions();

    //refresh on load
    QTimer::singleShot(3000, this, &LookupTelemetry::defaultLookup);
}
//==============================================================================
void LookupTelemetry::updateActions()
{
    quint64 num = recordNum();
    quint64 cnt = recordsCount();
    quint64 id = recordId();
    f_prev->setEnabled(num > 1);
    f_next->setEnabled(num && num < cnt);
    f_remove->setEnabled(id);
    if (cnt == 0) {
        setRecordId(0);
        setRecordNum(0);
    }
}
void LookupTelemetry::updateStatus()
{
    setStatus(QString("%1/%2").arg(recordNum()).arg(recordsCount()));
}
//==============================================================================
void LookupTelemetry::loadItem(QVariantMap modelData)
{
    quint64 key = modelData.value("key", 0).toUInt();
    if (!key)
        return;
    setRecordTimestamp(modelData.value("time").toULongLong());
    setRecordId(key);
    emit recordTriggered(recordId());
}
//=============================================================================
void LookupTelemetry::jumpToRecord(quint64 v)
{
    quint64 id = recordId();
    setRecordId(v);
    if (id != recordId())
        emit recordTriggered(recordId());
}
//=============================================================================
bool LookupTelemetry::fixItemDataThr(QVariantMap *item)
{
    QString time = QDateTime::fromMSecsSinceEpoch(item->value("time").toLongLong())
                       .toString("yyyy MMM dd hh:mm:ss");
    QString callsign = item->value("callsign").toString();
    QString comment = item->value("comment").toString();
    QString notes = item->value("notes").toString();
    QString total;
    quint64 t = item->value("totalTime").toULongLong();
    if (t > 0)
        total = AppRoot::timeToString(t / 1000);
    QStringList descr;
    if (!comment.isEmpty())
        descr << comment;
    if (!notes.isEmpty())
        descr << notes;
    QStringList status;
    if (!callsign.isEmpty())
        status << callsign;
    if (!total.isEmpty())
        status << total;

    item->insert("title", time);
    item->insert("status", status.join(' '));
    item->insert("descr", descr.join(" - "));
    //active current
    item->insert("active", item->value("key").toULongLong() == recordId());
    return true;
}
//=============================================================================
QString LookupTelemetry::filterQuery() const
{
    return "( callsign LIKE ? OR notes LIKE ? OR comment LIKE ? )";
}
QVariantList LookupTelemetry::filterValues() const
{
    const QString sf = QString("%%%1%%").arg(filter());
    return QVariantList() << sf << sf << sf;
}
QString LookupTelemetry::filterTrash() const
{
    return "trash IS NULL";
}
//=============================================================================
//=============================================================================
void LookupTelemetry::defaultLookup()
{
    //qDebug()<<filter();
    query("SELECT * FROM Telemetry"
          " LEFT JOIN TelemetryStats ON TelemetryStats.telemetryID=Telemetry.key"
          " WHERE "
              + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
              + " ORDER BY time DESC, key DESC",
          filter().isEmpty() ? QVariantList() : filterValues());
    //find count
    QString qs = "SELECT COUNT(*) FROM Telemetry"
                 " WHERE "
                 + filterTrash() + " AND " + filterQuery();
    DatabaseRequest *req = new DatabaseRequest(db, qs, filterValues());
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsLookup,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbResultsLookup(DatabaseRequest::Records records)
{
    setRecordsCount(records.values.isEmpty() ? 0 : records.values.first().first().toULongLong());
}
//=============================================================================
void LookupTelemetry::dbLoadInfo()
{
    quint64 key = recordId();
    if (!key)
        return;
    QString qs = "SELECT * FROM Telemetry"
                 " LEFT JOIN TelemetryStats ON TelemetryStats.telemetryID=Telemetry.key"
                 " WHERE Telemetry.key=?";
    DatabaseRequest *req = new DatabaseRequest(db, qs, QVariantList() << recordId());
    connect(this, &LookupTelemetry::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsInfo,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbResultsInfo(DatabaseRequest::Records records)
{
    if (records.values.isEmpty())
        return;
    QVariantMap info = DatabaseRequest::queryRecord(records);
    setRecordTimestamp(info.value("time").toULongLong());
    jumpToRecord(info.value("key").toULongLong());
    info.remove("key");
    info.remove("telemetryID");
    setRecordInfo(info);
}
//=============================================================================
void LookupTelemetry::dbFindNum()
{
    quint64 key = recordId();
    if (!key)
        return;
    QString qs = "SELECT ? AS key,COUNT(*) FROM Telemetry"
                 " WHERE "
                 + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
                 + " AND time<=?";
    DatabaseRequest *req = new DatabaseRequest(db,
                                               qs,
                                               QVariantList()
                                                   << key
                                                   << (filter().isEmpty() ? QVariantList()
                                                                          : filterValues())
                                                   << recordTimestamp());
    connect(this, &LookupTelemetry::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsNum,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbResultsNum(DatabaseRequest::Records records)
{
    if (records.values.isEmpty())
        return;
    quint64 key = records.values.first().at(0).toULongLong();
    if (key != recordId())
        return;
    quint64 num = records.values.first().at(1).toULongLong();
    setRecordNum(num);
    if (!num)
        return;
    QString qs = "SELECT ? AS cur,* FROM Telemetry"
                 " WHERE "
                 + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
                 + " AND time<=?"
                   " ORDER BY time DESC, key DESC LIMIT 100";
    DatabaseRequest *req = new DatabaseRequest(db,
                                               qs,
                                               QVariantList()
                                                   << key
                                                   << (filter().isEmpty() ? QVariantList()
                                                                          : filterValues())
                                                   << recordTimestamp());
    connect(this, &LookupTelemetry::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsNumNext,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbResultsNumNext(DatabaseRequest::Records records)
{
    if (records.values.isEmpty())
        return;
    quint64 key = records.values.first().at(0).toULongLong();
    if (key != recordId())
        return;
    quint64 num = recordNum();
    for (int i = 0; i < records.values.size(); ++i) {
        if (records.values.at(i).at(1).toULongLong() == key)
            break;
        num--;
    }
    setRecordNum(num);
}
//=============================================================================
//=============================================================================
void LookupTelemetry::dbLoadLatest()
{
    emit discardRequests();
    defaultLookup();
    QString qs = "SELECT * FROM Telemetry"
                 " WHERE "
                 + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
                 + " ORDER BY Telemetry.time DESC, Telemetry.key DESC LIMIT 1";
    DatabaseRequest *req = new DatabaseRequest(db,
                                               qs,
                                               filter().isEmpty() ? QVariantList() : filterValues());
    connect(this, &LookupTelemetry::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsLatest,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbResultsLatest(DatabaseRequest::Records records)
{
    if (records.values.isEmpty())
        return;
    const QStringList &n = records.names;
    const QVariantList &r = records.values.first();
    setRecordTimestamp(r.at(n.indexOf("time")).toULongLong());
    setRecordId(r.at(0).toULongLong());
    emit recordTriggered(recordId());
}
//=============================================================================
void LookupTelemetry::dbLoadPrev()
{
    emit discardRequests();
    if (!recordId()) {
        dbLoadLatest();
        return;
    }
    QString qs = "SELECT * FROM Telemetry"
                 " WHERE "
                 + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
                 + " AND Telemetry.time<=?"
                   " ORDER BY Telemetry.time DESC, Telemetry.key DESC LIMIT 100";
    DatabaseRequest *req = new DatabaseRequest(db,
                                               qs,
                                               (filter().isEmpty() ? QVariantList() : filterValues())
                                                   << recordTimestamp());
    connect(this, &LookupTelemetry::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsPrevNext,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbLoadNext()
{
    emit discardRequests();
    if (!recordId()) {
        dbLoadLatest();
        return;
    }
    QString qs = "SELECT * FROM Telemetry"
                 " WHERE "
                 + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
                 + " AND Telemetry.time>=?"
                   " ORDER BY Telemetry.time ASC, Telemetry.key ASC LIMIT 100";
    DatabaseRequest *req = new DatabaseRequest(db,
                                               qs,
                                               (filter().isEmpty() ? QVariantList() : filterValues())
                                                   << recordTimestamp());
    connect(this, &LookupTelemetry::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &LookupTelemetry::dbResultsPrevNext,
            Qt::QueuedConnection);
    req->exec();
}
void LookupTelemetry::dbRemove()
{
    quint64 key = recordId();
    if (!key)
        return;
    quint64 num = recordNum();
    if (num > 0)
        f_prev->trigger();
    QVariantMap info;
    info.insert("trash", 1);
    DBReqTelemetryWriteInfo *req = new DBReqTelemetryWriteInfo(key, info);
    if (num <= 0)
        connect(
            req,
            &DBReqTelemetryWriteInfo::finished,
            this,
            [this]() { f_latest->trigger(); },
            Qt::QueuedConnection);
    //else connect(req,&DBReqRemove::finished,this,&TelemetryReader::rescan,Qt::QueuedConnection);
    req->exec();
}
//=============================================================================
void LookupTelemetry::dbResultsPrevNext(DatabaseRequest::Records records)
{
    if (records.values.isEmpty())
        return;
    const QStringList &n = records.names;
    quint64 cur = recordId();
    const int cnt = records.values.size() - 1;
    for (int i = 0; i <= cnt; ++i) {
        quint64 key = records.values.at(i).at(0).toULongLong();
        //qDebug()<<i<<key<<cur<<cnt;
        if (key != cur)
            continue;
        if (i == cnt)
            break;
        const QVariantList &r = records.values.at(i + 1);
        setRecordTimestamp(r.at(n.indexOf("time")).toULongLong());
        jumpToRecord(r.at(0).toULongLong());
        return;
    }
}
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
quint64 LookupTelemetry::recordsCount() const
{
    return m_recordsCount;
}
void LookupTelemetry::setRecordsCount(quint64 v)
{
    if (m_recordsCount == v)
        return;
    m_recordsCount = v;
    emit recordsCountChanged();
}
quint64 LookupTelemetry::recordNum() const
{
    return m_recordNum;
}
void LookupTelemetry::setRecordNum(quint64 v)
{
    if (m_recordNum == v)
        return;
    m_recordNum = v;
    emit recordNumChanged();
}
quint64 LookupTelemetry::recordId()
{
    QMutexLocker lock(&mutexRecordId);
    return m_recordId;
}
void LookupTelemetry::setRecordId(quint64 v)
{
    mutexRecordId.lock();
    if (m_recordId == v) {
        mutexRecordId.unlock();
        return;
    }
    m_recordId = v;
    mutexRecordId.unlock();
    emit recordIdChanged();
}
quint64 LookupTelemetry::recordTimestamp() const
{
    return m_recordTimestamp;
}
void LookupTelemetry::setRecordTimestamp(quint64 v)
{
    if (m_recordTimestamp == v)
        return;
    m_recordTimestamp = v;
    emit recordTimestampChanged();
}
QVariantMap LookupTelemetry::recordInfo() const
{
    return m_recordInfo;
}
void LookupTelemetry::setRecordInfo(const QVariantMap &v)
{
    //if(m_recordInfo==v)return;
    m_recordInfo = v;
    emit recordInfoChanged();
}
//=============================================================================
