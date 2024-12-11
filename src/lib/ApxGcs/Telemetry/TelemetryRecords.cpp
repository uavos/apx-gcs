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

#include <Database/DatabaseModel.h>
#include <Database/StorageReq.h>

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
    connect(_dbmodel, &DatabaseModel::itemTriggered, this, &TelemetryRecords::recordTriggered);
    connect(_dbmodel, &DatabaseModel::itemTriggered, this, [this](quint64 id) {
        setRecordNum(_dbmodel->recordsList().indexOf(id) + 1);
    });
    connect(_dbmodel, &DatabaseModel::recordsListChanged, this, [this]() {
        setRecordNum(_dbmodel->recordsList().indexOf(_dbmodel->activeRecordId()) + 1);
    });

    connect(this, &Fact::triggered, this, &TelemetryRecords::dbRequestRecordsList);
    // connect(Database::instance()->telemetry,
    //         &DatabaseSession::modified,
    //         this,
    //         &TelemetryRecords::dbRequestRecordsList);

    //actions
    f_remove = new Fact(this,
                        "remove",
                        tr("Remove"),
                        tr("Remove current record"),
                        Action | ShowDisabled | Remove | IconOnly,
                        "delete");
    connect(f_remove, &Fact::triggered, this, &TelemetryRecords::dbRemove);

    f_restore = new Fact(this,
                         "undelete",
                         tr("Restore"),
                         tr("Show records from trash"),
                         Action | Bool | IconOnly,
                         "delete-restore");
    connect(f_restore, &Fact::valueChanged, this, &TelemetryRecords::dbRequestRecordsList);

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

    f_latest = new Fact(this,
                        "latest",
                        tr("Latest"),
                        tr("Load latest"),
                        Action | ShowDisabled | Apply | IconOnly,
                        "fast-forward");
    connect(f_latest, &Fact::triggered, this, &TelemetryRecords::dbLoadLatest);

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
    f_next->setEnabled(!num || num > 1);
    f_prev->setEnabled(!num || num < cnt);
    f_remove->setEnabled(num > 0 && cnt > 0);
    if (cnt == 0) {
        setRecordNum(0);
    }
}
void TelemetryRecords::updateStatus()
{
    setValue(QString("%1/%2").arg(recordNum()).arg(recordsCount()));
}

void TelemetryRecords::dbRequestRecordsList()
{
    QStringList fields = {"unitName", "unitType", "confName", "notes", "file"};
    auto extra_filter = f_restore->value().toBool() ? "" : QString("trash IS NULL");
    auto filter = _dbmodel->getFilterExpression(fields, extra_filter);

    QString s = "SELECT key, time FROM Telemetry";
    if (!filter.isEmpty())
        s += " WHERE " + filter;
    s += " ORDER BY time DESC";

    auto req = new db::storage::Request(s, {});
    connect(req,
            &db::storage::Request::queryResults,
            _dbmodel,
            &DatabaseModel::dbUpdateRecords,
            Qt::QueuedConnection);
    req->exec();
}

void TelemetryRecords::dbRequestRecordInfo(quint64 id)
{
    auto req = new db::storage::TelemetryLoadInfo(id);
    connect(req,
            &db::storage::TelemetryLoadInfo::modelInfo,
            _dbmodel,
            &DatabaseModel::setRecordModelInfo);
    req->exec();
}

void TelemetryRecords::dbLoadLatest()
{
    emit discardRequests();

    connect(
        _dbmodel,
        &DatabaseModel::recordsListChanged,
        this,
        [this]() { _dbmodel->triggerItem(_dbmodel->recordsList().value(0)); },
        Qt::SingleShotConnection);

    if (_dbmodel->filter().isEmpty()) {
        dbRequestRecordsList();
    } else {
        _dbmodel->resetFilter();
    }
}

void TelemetryRecords::dbLoadNext()
{
    emit discardRequests();

    const auto &recordsList = _dbmodel->recordsList();
    int num = recordsList.indexOf(_dbmodel->activeRecordId());
    setRecordNum(num + 1);
    if (num == 0)
        return;
    if (num < 0)
        num = 1;
    _dbmodel->triggerItem(recordsList.value(num - 1));
}
void TelemetryRecords::dbLoadPrev()
{
    emit discardRequests();

    const auto &recordsList = _dbmodel->recordsList();
    int num = recordsList.indexOf(_dbmodel->activeRecordId());
    setRecordNum(num + 1);
    if (num >= recordsList.size() - 1)
        return;
    _dbmodel->triggerItem(recordsList.value(num + 1));
}
void TelemetryRecords::dbRemove()
{
    emit discardRequests();

    auto id = _dbmodel->activeRecordId();
    if (!id)
        return;

    auto req = new db::storage::TelemetryModifyTrash(id);
    connect(req, &db::storage::TelemetryModifyTrash::finished, [this, id]() {
        dbRequestRecordsList();
        dbRequestRecordInfo(id);
        dbLoadNext();
    });
    req->exec();
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
