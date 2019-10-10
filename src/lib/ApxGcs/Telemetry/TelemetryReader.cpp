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
#include "TelemetryReader.h"
#include "LookupTelemetry.h"
#include <App/App.h>
#include <App/AppLog.h>
#include <App/AppRoot.h>

#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>

#include <QGeoCoordinate>
//=============================================================================
TelemetryReader::TelemetryReader(LookupTelemetry *lookup, Fact *parent)
    : Fact(parent, "reader", "", "", Group, "progress-download")
    , lookup(lookup)
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
    connect(f_reload, &Fact::triggered, this, &TelemetryReader::reloadTriggered);

    //status
    connect(lookup, &LookupTelemetry::recordInfoChanged, this, &TelemetryReader::updateRecordInfo);
    connect(this, &TelemetryReader::totalTimeChanged, this, &TelemetryReader::updateStatus);

    //load sequence
    qRegisterMetaType<fieldData_t>("fieldData_t");
    qRegisterMetaType<fieldNames_t>("fieldNames_t");
    qRegisterMetaType<times_t>("times_t");
    qRegisterMetaType<events_t>("events_t");
    qRegisterMetaType<FactList>("FactList");

    connect(lookup, &LookupTelemetry::recordTriggered, this, &TelemetryReader::load);

    connect(&loadEvent, &DelayedEvent::triggered, this, &TelemetryReader::dbLoadData);
    loadEvent.setInterval(500);

    connect(this, &Fact::triggered, this, &TelemetryReader::loadCurrent);

    updateRecordInfo();
}
//==============================================================================
void TelemetryReader::updateStatus()
{
    QStringList st;
    st << AppRoot::timeToString(totalTime() / 1000, true);
    setStatus(st.join('/'));
}
//==============================================================================
void TelemetryReader::loadCurrent()
{
    if (!lookup->recordId())
        lookup->f_latest->trigger();
}
//==============================================================================
void TelemetryReader::updateRecordInfo()
{
    QVariantMap info = lookup->recordInfo();
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
//==============================================================================
void TelemetryReader::load()
{
    f_reload->setEnabled(false);
    quint64 key = lookup->recordId();
    if (!key)
        return;
    setTotalSize(0);
    setTotalTime(0);
    setProgress(0);
    removeAll();
    DBReqTelemetryFindCache *req = new DBReqTelemetryFindCache(key);
    connect(req,
            &DBReqTelemetryFindCache::cacheFound,
            this,
            &TelemetryReader::dbCacheFound,
            Qt::QueuedConnection);
    connect(req,
            &DBReqTelemetryFindCache::cacheNotFound,
            this,
            &TelemetryReader::dbCacheNotFound,
            Qt::QueuedConnection);
    req->exec();
}
//==============================================================================
void TelemetryReader::reloadTriggered()
{
    quint64 key = lookup->recordId();
    if (!key)
        return;
    Database::instance()->telemetry->markCacheInvalid(key);
    load();
}
//==============================================================================
//==============================================================================
void TelemetryReader::dbCacheNotFound(quint64 telemetryID)
{
    if (telemetryID != lookup->recordId())
        return;
    loadEvent.schedule();
}
void TelemetryReader::dbCacheFound(quint64 telemetryID)
{
    if (telemetryID != lookup->recordId())
        return;
    dbLoadData();
}
void TelemetryReader::dbLoadData()
{
    quint64 telemetryID = lookup->recordId();
    if (!telemetryID)
        return;
    setProgress(0);
    //request data
    {
        DBReqTelemetryMakeStats *req = new DBReqTelemetryMakeStats(telemetryID);
        connect(lookup,
                &LookupTelemetry::discardRequests,
                req,
                &DatabaseRequest::discard,
                Qt::QueuedConnection);
        connect(req,
                &DBReqTelemetryMakeStats::statsUpdated,
                this,
                &TelemetryReader::dbStatsUpdated,
                Qt::QueuedConnection);
        connect(req,
                &DBReqTelemetryMakeStats::statsFound,
                this,
                &TelemetryReader::dbStatsFound,
                Qt::QueuedConnection);
        req->exec();
    }
}
//=============================================================================
//=============================================================================
void TelemetryReader::dbStatsFound(quint64 telemetryID, QVariantMap stats)
{
    Q_UNUSED(stats)
    if (telemetryID != lookup->recordId())
        return;
    TelemetryReaderDataReq *req = new TelemetryReaderDataReq(telemetryID);
    connect(lookup,
            &LookupTelemetry::discardRequests,
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
    req->exec();
}
void TelemetryReader::dbStatsUpdated(quint64 telemetryID, QVariantMap stats)
{
    if (telemetryID != lookup->recordId())
        return;
    QVariantMap info = lookup->recordInfo();
    foreach (QString key, stats.keys()) {
        info[key] = stats.value(key);
    }
    lookup->setRecordInfo(info);
    updateRecordInfo();
    dbStatsFound(telemetryID, stats);
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
    if (telemetryID != lookup->recordId())
        return;

    removeAll();
    f_reload->setEnabled(true);

    times.swap(this->times);
    fieldNames.swap(this->fieldNames);
    fieldData.swap(this->fieldData);
    events.swap(this->events);
    this->geoPath = path;

    /*for (int i = 0; i < this->events.size(); ++i) {
        const event_t &e = this->events.at(i);
        addEventFact(e.time, e.name, e.value, e.uid);
    }*/
    f_events->moveToThread(thread());
    f_events->setParentFact(this);
    for (int i = 0; i < f_events->size(); ++i) {
        Fact *g = f_events->child(i);
        for (int j = 0; j < g->size(); ++j) {
            Fact *f = g->child(j);
            connect(f, &Fact::triggered, this, [this, f]() { emit recordFactTriggered(f); });
        }
    }
    //App::jsync(this);

    quint64 tMax = 0;
    if (!this->times.isEmpty())
        tMax = qRound(this->times.last() * 1000.0);
    setTotalTime(tMax);

    setProgress(-1);
    emit dataAvailable(cacheID);
}
void TelemetryReader::dbProgress(quint64 telemetryID, int v)
{
    if (telemetryID != lookup->recordId())
        return;
    setProgress(v);
}
//=============================================================================
//=============================================================================
void TelemetryReader::notesChanged()
{
    if (blockNotesChange)
        return;
    if (!lookup->recordId())
        return;
    QVariantMap info;
    info.insert("notes", f_notes->text());
    DBReqTelemetryWriteInfo *req = new DBReqTelemetryWriteInfo(lookup->recordId(), info);
    connect(
        req,
        &DBReqTelemetryWriteInfo::finished,
        this,
        [=]() { apxMsg() << tr("Notes recorded").append(':') << title(); },
        Qt::QueuedConnection);
    req->exec();
}
//=============================================================================
//=============================================================================
//=============================================================================
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
//=============================================================================
