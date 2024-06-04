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
#include "TelemetryFiles.h"

#include <App/AppRoot.h>

TelemetryFiles::TelemetryFiles(Fact *parent)
    : Fact(parent, "files", tr("Files"), tr("Telemetry files lookup"), FilterModel, "database-search")
    , m_recordsCount(0)
    , m_recordNum(0)
    , m_recordTimestamp(0)
{
    // get ms since epoch UTC at 2020-01-01 00:00:00
    auto epoch = QDateTime(QDate(2020, 1, 1), QTime(0, 0, 0)).toMSecsSinceEpoch();
    qWarning() << "epoch" << epoch;

    setOpt("pos", QPointF(1, 1));

    setOpt("page", "Menu/FactMenuPageLookupDB.qml");

    _worker = new TelemetryFilesWorker(this);
    _filesModel = new TelemetryFilesModel(_worker, AppDirs::telemetry().absolutePath(), this);

    setModel(_filesModel);

    connect(_filesModel, &TelemetryFilesModel::countChanged, this, [this]() {
        setRecordsCount(_filesModel->count());
    });

    //actions
    f_latest = new Fact(this,
                        "latest",
                        tr("Latest"),
                        tr("Load latest"),
                        Action | ShowDisabled | Apply | IconOnly,
                        "fast-forward");
    // connect(f_latest, &Fact::triggered, this, &TelemetryFiles::dbLoadLatest);

    f_prev = new Fact(this,
                      "prev",
                      tr("Prev"),
                      tr("Load previous"),
                      Action | ShowDisabled | IconOnly,
                      "chevron-left");
    // connect(f_prev, &Fact::triggered, this, &TelemetryFiles::dbLoadPrev);

    f_next = new Fact(this,
                      "next",
                      tr("Next"),
                      tr("Load next"),
                      Action | ShowDisabled | IconOnly,
                      "chevron-right");
    // connect(f_next, &Fact::triggered, this, &TelemetryFiles::dbLoadNext);

    f_remove = new Fact(this,
                        "remove",
                        tr("Remove"),
                        tr("Remove current record"),
                        Action | ShowDisabled | Remove | IconOnly,
                        "delete");
    // connect(f_remove, &Fact::triggered, this, &TelemetryFiles::dbRemove);

    //totals
    connect(this, &TelemetryFiles::recordsCountChanged, this, &TelemetryFiles::updateStatus);
    connect(this, &TelemetryFiles::recordNumChanged, this, &TelemetryFiles::updateStatus);

    //actions update
    connect(this, &TelemetryFiles::recordNumChanged, this, &TelemetryFiles::updateActions);
    connect(this, &TelemetryFiles::recordsCountChanged, this, &TelemetryFiles::updateActions);
    updateActions();

    //refresh on load
    // QTimer::singleShot(3000, this, &TelemetryFiles::defaultLookup);
}
void TelemetryFiles::updateActions()
{
    quint64 num = recordNum();
    quint64 cnt = recordsCount();
    f_prev->setEnabled(num > 1);
    f_next->setEnabled(num && num < cnt);
    // f_remove->setEnabled(id);
    if (cnt == 0) {
        setRecordNum(0);
    }
}
void TelemetryFiles::updateStatus()
{
    setValue(QString("%1/%2").arg(recordNum()).arg(recordsCount()));
}

void TelemetryFiles::triggerItem(QVariantMap modelData)
{
    auto id = modelData["id"].toULongLong();
    _filesModel->setActiveId(id);

    setRecordNum(id);
    // setRecordInfo(modelData);
    // setRecordTimestamp(modelData.value("timestamp").toULongLong());

    auto path = AppDirs::telemetry().absoluteFilePath(
        modelData["name"].toString().append('.').append(telemetry::APXTLM_FTYPE));

    emit loadfile(path);

    auto job = new TelemetryFilesJobParse(_worker, path, id);

    connect(&job->reader, &TelemetryFileReader::progressChanged, this, &TelemetryFiles::setProgress);
    connect(&job->reader,
            &TelemetryFileReader::infoUpdated,
            _filesModel,
            [this, id](QJsonObject info) { _filesModel->updateFileInfo(info, id); });

    job->schedule();
}

// PROPERTIES

quint64 TelemetryFiles::recordsCount() const
{
    return m_recordsCount;
}
void TelemetryFiles::setRecordsCount(quint64 v)
{
    if (m_recordsCount == v)
        return;
    m_recordsCount = v;
    emit recordsCountChanged();
}
quint64 TelemetryFiles::recordNum() const
{
    return m_recordNum;
}
void TelemetryFiles::setRecordNum(quint64 v)
{
    if (m_recordNum == v)
        return;
    m_recordNum = v;
    emit recordNumChanged();
}
quint64 TelemetryFiles::recordTimestamp() const
{
    return m_recordTimestamp;
}
void TelemetryFiles::setRecordTimestamp(quint64 v)
{
    if (m_recordTimestamp == v)
        return;
    m_recordTimestamp = v;
    emit recordTimestampChanged();
}
QVariantMap TelemetryFiles::recordInfo() const
{
    return m_recordInfo;
}
void TelemetryFiles::setRecordInfo(const QVariantMap &v)
{
    //if(m_recordInfo==v)return;
    m_recordInfo = v;
    emit recordInfoChanged();
}
