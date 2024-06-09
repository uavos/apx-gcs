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
#include "TelemetryRecords.h"
#include "TelemetryDBReq.h"

#include <Database/Database.h>
#include <Database/TelemetryDB.h>

#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>

#include <App/AppRoot.h>

TelemetryRecords::TelemetryRecords(Fact *parent)
    : Fact(parent, "records", tr("Records"), tr("Telemetry records"), FilterModel, "database-search")
{
    setOpt("pos", QPointF(1, 1));

    setOpt("page", "Menu/FactMenuPageLookupDB.qml");

    _dbmodel = new DatabaseModel(this);
    setModel(_dbmodel);

    connect(_dbmodel,
            &DatabaseModel::requestRecordsList,
            this,
            &TelemetryRecords::dbRequestRecordsList);
    connect(_dbmodel,
            &DatabaseModel::requestRecordInfo,
            this,
            &TelemetryRecords::dbRequestRecordInfo);
    connect(_dbmodel, &DatabaseModel::countChanged, this, &TelemetryRecords::setRecordsCount);

    connect(this, &Fact::triggered, this, &TelemetryRecords::dbRequestRecordsList);

    // connect(this, &DatabaseLookup::itemTriggered, this, &TelemetryRecords::loadItem);

    //actions
    f_restore = new Fact(this,
                         "undelete",
                         tr("Restore"),
                         tr("Show records from trash"),
                         Action | Bool | IconOnly,
                         "delete-restore");
    // connect(f_restore, &Fact::valueChanged, this, &TelemetryRecords::defaultLookup);

    f_latest = new Fact(this,
                        "latest",
                        tr("Latest"),
                        tr("Load latest"),
                        Action | ShowDisabled | Apply | IconOnly,
                        "fast-forward");
    connect(f_latest, &Fact::triggered, this, &TelemetryRecords::dbLoadLatest);

    f_prev = new Fact(this,
                      "prev",
                      tr("Prev"),
                      tr("Load previous"),
                      Action | ShowDisabled | IconOnly,
                      "chevron-left");
    connect(f_prev, &Fact::triggered, this, &TelemetryRecords::dbLoadPrev);

    f_next = new Fact(this,
                      "next",
                      tr("Next"),
                      tr("Load next"),
                      Action | ShowDisabled | IconOnly,
                      "chevron-right");
    connect(f_next, &Fact::triggered, this, &TelemetryRecords::dbLoadNext);

    f_remove = new Fact(this,
                        "remove",
                        tr("Remove"),
                        tr("Remove current record"),
                        Action | ShowDisabled | Remove | IconOnly,
                        "delete");
    connect(f_remove, &Fact::triggered, this, &TelemetryRecords::dbRemove);

    // status totals
    connect(this, &TelemetryRecords::recordsCountChanged, this, &TelemetryRecords::updateStatus);
    connect(this, &TelemetryRecords::recordNumChanged, this, &TelemetryRecords::updateStatus);

    // actions update
    connect(this, &TelemetryRecords::recordNumChanged, this, &TelemetryRecords::updateActions);
    connect(this, &TelemetryRecords::recordsCountChanged, this, &TelemetryRecords::updateActions);
    updateActions();
}
void TelemetryRecords::updateActions()
{
    quint64 num = recordNum();
    quint64 cnt = recordsCount();
    f_prev->setEnabled(num > 1);
    f_next->setEnabled(num && num < cnt);
    f_remove->setEnabled(_selectedRecordId > 0);
    if (cnt == 0) {
        _selectedRecordId = 0;
        setRecordNum(0);
    }
}
void TelemetryRecords::updateStatus()
{
    setValue(QString("%1/%2").arg(recordNum()).arg(recordsCount()));
}

void TelemetryRecords::dbRequestRecordsList()
{
    auto filter = _dbmodel->getFilterExpression({"callsign", "notes", "comment", "file"});
    auto req = new DBReqTelemetryModelRecordsList(filter);
    connect(req,
            &DBReqTelemetryModelRecordsList::recordsList,
            _dbmodel,
            &DatabaseModel::setRecordsList);
    req->exec();
}

void TelemetryRecords::dbRequestRecordInfo(quint64 id)
{
    auto req = new DBReqTelemetryModelRecordInfo(id);
    connect(req,
            &DBReqTelemetryModelRecordInfo::recordInfo,
            _dbmodel,
            &DatabaseModel::setRecordInfo);
    req->exec();
}

/*bool TelemetryRecords::fixItemDataThr(QVariantMap *item)
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
    if (item->value("trash").toBool())
        descr << tr("deleted").toUpper();

    if (!comment.isEmpty())
        descr << comment;
    if (!notes.isEmpty())
        descr << notes;
    QStringList value;
    if (!callsign.isEmpty())
        value << callsign;
    if (!total.isEmpty())
        value << total;

    item->insert("title", time);
    item->insert("descr", descr.join(" - "));
    item->insert("value", value.join(' '));
    //active current
    item->insert("active", item->value("key").toULongLong() == _selectedRecordId);
    return true;
}*/

void TelemetryRecords::setActiveRecordId(quint64 id)
{
    if (id == _selectedRecordId)
        return;
    _selectedRecordId = id;
}

QString TelemetryRecords::filterQuery() const
{
    return "( callsign LIKE ? OR notes LIKE ? OR comment LIKE ? )";
}
QVariantList TelemetryRecords::filterValues() const
{
    // const QString sf = QString("%%%1%%").arg(filter());
    // return QVariantList() << sf << sf << sf;
    return {};
}
QString TelemetryRecords::filterTrash() const
{
    return f_restore->value().toBool() ? "TRUE" : "trash IS NULL";
}

/*void TelemetryRecords::defaultLookup()
{
    // qDebug() << filter();

    query("SELECT * FROM Telemetry"
          " WHERE "
              + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
              + " ORDER BY time DESC, key DESC",
          filter().isEmpty() ? QVariantList() : filterValues());

    //find records count
    QString qs = "SELECT COUNT(*) FROM Telemetry"
                 " WHERE "
                 + filterTrash() + " AND " + filterQuery();
    DatabaseRequest *req = new DatabaseRequest(db, qs, filterValues());
    connect(req, &DatabaseRequest::queryResults, this, [this]() {
        setRecordsCount(records.values.isEmpty() ? 0 : records.values.first().first().toULongLong());
    });
    req->exec();
}*/

void TelemetryRecords::dbFindNum()
{
    /*quint64 key = recordId();
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
    connect(this, &TelemetryRecords::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &TelemetryRecords::dbResultsNum,
            Qt::QueuedConnection);
    req->exec();*/
}
void TelemetryRecords::dbResultsNum(DatabaseRequest::Records records)
{
    /*if (records.values.isEmpty())
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
    connect(this, &TelemetryRecords::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &TelemetryRecords::dbResultsNumNext,
            Qt::QueuedConnection);
    req->exec();*/
}
void TelemetryRecords::dbResultsNumNext(DatabaseRequest::Records records)
{
    /*if (records.values.isEmpty())
        return;
    quint64 key = records.values.first().at(0).toULongLong();
    if (key != _selectedRecordId)
        return;
    quint64 num = recordNum();
    for (int i = 0; i < records.values.size(); ++i) {
        if (records.values.at(i).at(1).toULongLong() == key)
            break;
        num--;
    }
    setRecordNum(num);*/
}

void TelemetryRecords::dbLoadLatest()
{
    /*emit discardRequests();
    defaultLookup();
    QString qs = "SELECT * FROM Telemetry"
                 " WHERE "
                 + filterTrash() + (filter().isEmpty() ? "" : " AND " + filterQuery())
                 + " ORDER BY Telemetry.time DESC, Telemetry.key DESC LIMIT 1";
    DatabaseRequest *req = new DatabaseRequest(db,
                                               qs,
                                               filter().isEmpty() ? QVariantList() : filterValues());
    connect(this, &TelemetryRecords::discardRequests, req, &DatabaseRequest::discard);
    connect(req, &DatabaseRequest::queryResults, this, [this](DatabaseRequest::Records records) {
        if (!records.values.isEmpty()) {
            const QStringList &n = records.names;
            const QVariantList &r = records.values.first();
            setRecordId(r.at(0).toULongLong());
            emit recordTriggered(recordId());
        }
    });
    req->exec();*/
}

void TelemetryRecords::dbLoadPrev()
{
    /*emit discardRequests();
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
    connect(this, &TelemetryRecords::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &TelemetryRecords::dbResultsPrevNext,
            Qt::QueuedConnection);
    req->exec();*/
}
void TelemetryRecords::dbLoadNext()
{
    /*emit discardRequests();
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
    connect(this, &TelemetryRecords::discardRequests, req, &DatabaseRequest::discard);
    connect(req,
            &DatabaseRequest::queryResults,
            this,
            &TelemetryRecords::dbResultsPrevNext,
            Qt::QueuedConnection);
    req->exec();*/
}
void TelemetryRecords::dbRemove()
{
    /*quint64 key = recordId();
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
    req->exec();*/
}

void TelemetryRecords::dbResultsPrevNext(DatabaseRequest::Records records)
{
    /*if (records.values.isEmpty())
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
        jumpToRecord(r.at(0).toULongLong());
        return;
    }*/
}

quint64 TelemetryRecords::recordsCount() const
{
    return m_recordsCount;
}
void TelemetryRecords::setRecordsCount(quint64 v)
{
    if (m_recordsCount == v)
        return;
    m_recordsCount = v;
    emit recordsCountChanged();
}
quint64 TelemetryRecords::recordNum() const
{
    return m_recordNum;
}
void TelemetryRecords::setRecordNum(quint64 v)
{
    if (m_recordNum == v)
        return;
    m_recordNum = v;
    emit recordNumChanged();
}
