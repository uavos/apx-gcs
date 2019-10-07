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
#ifndef TelemetryReader_H
#define TelemetryReader_H
#include <ApxMisc/DelayedEvent.h>
#include <Database/DatabaseRequest.h>
#include <Fact/Fact.h>
#include <QGeoPath>
#include <QtCore>

#include "TelemetryReaderDataReq.h"
class LookupTelemetry;
//=============================================================================
class TelemetryReader : public Fact
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalSize READ totalSize NOTIFY totalSizeChanged)
    Q_PROPERTY(quint64 totalTime READ totalTime NOTIFY totalTimeChanged)

public:
    explicit TelemetryReader(LookupTelemetry *lookup, Fact *parent);

    LookupTelemetry *lookup;

    Fact *f_notes;
    Fact *f_reload;

    //data from database
    typedef TelemetryReaderDataReq::fieldData_t fieldData_t;
    typedef TelemetryReaderDataReq::evtCountMap_t evtCountMap_t;
    typedef TelemetryReaderDataReq::fieldNames_t fieldNames_t;
    typedef TelemetryReaderDataReq::times_t times_t;
    typedef TelemetryReaderDataReq::event_t event_t;
    typedef TelemetryReaderDataReq::events_t events_t;

    fieldData_t fieldData;
    evtCountMap_t evtCountMap;
    fieldNames_t fieldNames;
    times_t times;
    events_t events;
    QGeoPath geoPath;

private:
    bool blockNotesChange;
    DelayedEvent loadEvent;

private slots:
    void notesChanged();
    void updateStatus();

    void updateRecordInfo();
    void load();

    //Database
private slots:
    void dbLoadData();

    void dbCacheFound(quint64 telemetryID);
    void dbCacheNotFound(quint64 telemetryID);

    void dbResultsDataProc(quint64 telemetryID,
                           quint64 cacheID,
                           fieldData_t fieldData,
                           fieldNames_t fieldNames,
                           times_t times,
                           events_t events,
                           QGeoPath path,
                           Fact *f_events);

    void dbStatsFound(quint64 telemetryID, QVariantMap stats);
    void dbStatsUpdated(quint64 telemetryID, QVariantMap stats);
    void dbProgress(quint64 telemetryID, int v);

    void reloadTriggered();

signals:
    void statsAvailable();
    void dataAvailable(quint64 cacheID);

    void recordFactTriggered(Fact *f);

public slots:
    void loadCurrent();

    //PROPERTIES
public:
    quint64 totalSize() const;
    void setTotalSize(quint64 v);
    quint64 totalTime() const;
    void setTotalTime(quint64 v);

private:
    quint64 m_totalSize;
    quint64 m_totalTime;

signals:
    void totalSizeChanged();
    void totalTimeChanged();
};
//=============================================================================
#endif
