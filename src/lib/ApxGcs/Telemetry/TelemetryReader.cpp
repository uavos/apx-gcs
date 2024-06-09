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
#include "TelemetryReader.h"
#include "TelemetryDBReq.h"
#include "TelemetryRecords.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>

#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>

#include <QGeoCoordinate>

TelemetryReader::TelemetryReader(Fact *parent)
    : Fact(parent, "reader", "", "", Group, "progress-download")
    , blockNotesChange(false)
    , m_totalSize(0)
    , m_totalTime(0)
{
    f_notes = new Fact(parent, "notes", tr("Notes"), tr("Current record notes"), Text);
    f_notes->setIcon("note-text");
    connect(f_notes, &Fact::valueChanged, this, &TelemetryReader::notesChanged);

    //actions
    f_reload = new Fact(this,
                        "reload",
                        tr("Reload"),
                        tr("Invalidate cache"),
                        Action | ShowDisabled,
                        "reload");
    f_reload->setEnabled(false);
    connect(f_reload, &Fact::triggered, this, &TelemetryReader::load);

    //info
    connect(this, &TelemetryReader::totalTimeChanged, this, &TelemetryReader::updateStatus);

    //load sequence
    qRegisterMetaType<fieldData_t>("fieldData_t");
    qRegisterMetaType<fieldNames_t>("fieldNames_t");
    qRegisterMetaType<times_t>("times_t");
    qRegisterMetaType<events_t>("events_t");
    qRegisterMetaType<FactList>("FactList");

    connect(this, &Fact::triggered, this, &TelemetryReader::loadCurrent);

    updateRecordInfo();
}

void TelemetryReader::updateStatus()
{
    const QString &s = AppRoot::timeToString(totalTime() / 1000, true);
    setValue(s);
}

void TelemetryReader::loadCurrent()
{
    // if (!records->recordId())
    //     records->f_latest->trigger();
}

void TelemetryReader::updateRecordInfo()
{
    QVariantMap info; // = records->recordInfo();
    setTotalTime(info.value("totalTime").toULongLong());
    quint64 downlink = info.value("downlink").toULongLong();
    quint64 uplink = info.value("uplink").toULongLong();
    quint64 events = info.value("events").toULongLong();
    setTotalSize(downlink + uplink + events);
    evtCountMap.clear();
    foreach (QString s, info.value("evtDetails").toString().split(',')) {
        if (s.isEmpty())
            continue;
        QString key = s.left(s.indexOf('='));
        QString v = s.mid(s.indexOf('=') + 1);
        evtCountMap.insert(key, v.toUInt());
    }
    if (uplink)
        evtCountMap.insert("uplink", uplink);

    qint64 t = info.value("time").toLongLong();
    QString title = t > 0 ? QDateTime::fromMSecsSinceEpoch(t).toString("yyyy MMM dd hh:mm:ss")
                          : tr("Telemetry Data");
    QString callsign = info.value("callsign").toString();
    QString comment = info.value("comment").toString();
    QString notes = info.value("notes").toString();
    QString stime = AppRoot::timeToString(totalTime() / 1000, true);

    QStringList descr;
    if (!callsign.isEmpty())
        descr.append(callsign);
    if (!comment.isEmpty() && comment != callsign)
        descr.append(comment);

    setTitle(title);
    setDescr(descr.join(" | "));
    blockNotesChange = true;
    f_notes->setValue(notes);
    blockNotesChange = false;

    emit statsAvailable();
}

void TelemetryReader::load()
{
    f_reload->setEnabled(false);

    quint64 key = _recordID;
    if (!_recordID)
        return;

    setTotalSize(0);
    setTotalTime(0);
    setProgress(0);
    deleteChildren();

    auto req = new DBReqTelemetryLoadFile(key);
    // connect(records, &TelemetryRecords::discardRequests, req, &DatabaseRequest::discard);
    connect(req, &DatabaseRequest::finished, this, [this](DatabaseRequest::Status) {
        setProgress(-1);
    });
    auto reader = req->reader();
    connect(reader, &TelemetryFileReader::progressChanged, this, [this](int v) { setProgress(v); });
    connect(reader, &TelemetryFileReader::infoUpdated, this, &TelemetryReader::fileInfoLoaded);

    req->exec();
}

void TelemetryReader::fileInfoLoaded(QJsonObject data)
{
    qDebug() << QJsonDocument(data).toJson(QJsonDocument::Indented);
}

void TelemetryReader::dbStatsFound(quint64 telemetryID, QVariantMap stats)
{
    /*Q_UNUSED(stats)
    if (telemetryID != records->recordId())
        return;
    TelemetryReaderDataReq *req = new TelemetryReaderDataReq(telemetryID);
    connect(records,
            &TelemetryRecords::discardRequests,
            req,
            &DatabaseRequest::discard,
            Qt::QueuedConnection);
    connect(req,
            &TelemetryReaderDataReq::dataProcessed,
            this,
            &TelemetryReader::dbResultsDataProc,
            Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryReadData::progress,
            this,
            &TelemetryReader::dbProgress,
            Qt::QueuedConnection);
    connect(req, &DatabaseRequest::finished, this, [this](DatabaseRequest::Status) {
        setProgress(-1);
    });
    req->exec();*/
}
void TelemetryReader::dbResultsDataProc(quint64 telemetryID,
                                        quint64 cacheID,
                                        fieldData_t fieldData,
                                        fieldNames_t fieldNames,
                                        times_t times,
                                        events_t events,
                                        QGeoPath path,
                                        Fact *f_events)
{
    /*if (telemetryID != records->recordId())
        return;

    deleteChildren();
    f_reload->setEnabled(true);

    times.swap(this->times);
    fieldNames.swap(this->fieldNames);
    fieldData.swap(this->fieldData);
    events.swap(this->events);
    this->geoPath = path;

    changeThread(f_events, thread());
    f_events->setParentFact(this);
    for (int i = 0; i < f_events->size(); ++i) {
        Fact *g = f_events->child(i);
        for (int j = 0; j < g->size(); ++j) {
            Fact *f = g->child(j);
            connect(f, &Fact::triggered, this, [this, f]() { emit recordFactTriggered(f); });
        }
    }

    quint64 tMax = 0;
    if (!this->times.isEmpty())
        tMax = qRound(this->times.last() * 1000.0);
    setTotalTime(tMax);

    setProgress(-1);
    emit dataAvailable(cacheID);*/
}
void TelemetryReader::dbProgress(quint64 telemetryID, int v)
{
    /*if (telemetryID != records->recordId())
        return;
    setProgress(v);*/
}
void TelemetryReader::changeThread(Fact *fact, QThread *thread)
{
    fact->moveToThread(thread);
    for (auto i : fact->facts())
        changeThread(i, thread);
}

void TelemetryReader::notesChanged()
{
    /*if (blockNotesChange)
        return;
    if (!records->recordId())
        return;
    QVariantMap info;
    info.insert("notes", f_notes->text());
    DBReqTelemetryWriteInfo *req = new DBReqTelemetryWriteInfo(records->recordId(), info);
    connect(
        req,
        &DBReqTelemetryWriteInfo::finished,
        this,
        [this]() { apxMsg() << tr("Notes recorded").append(':') << title(); },
        Qt::QueuedConnection);
    req->exec();*/
}

quint64 TelemetryReader::totalSize() const
{
    return m_totalSize;
}
void TelemetryReader::setTotalSize(quint64 v)
{
    if (m_totalSize == v)
        return;
    m_totalSize = v;
    emit totalSizeChanged();
}
quint64 TelemetryReader::totalTime() const
{
    return m_totalTime;
}
void TelemetryReader::setTotalTime(quint64 v)
{
    if (m_totalTime == v)
        return;
    m_totalTime = v;
    emit totalTimeChanged();
}
