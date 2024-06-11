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
#pragma once

#include <ApxMisc/DelayedEvent.h>
#include <Database/DatabaseRequest.h>
#include <Fact/Fact.h>
#include <QGeoPath>
#include <QtCore>

#include "TelemetryFileReader.h"

#include "TelemetryReaderDataReq.h"
class TelemetryRecords;

class TelemetryReader : public Fact
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalSize READ totalSize NOTIFY totalSizeChanged)
    Q_PROPERTY(quint64 totalTime READ totalTime NOTIFY totalTimeChanged)

public:
    explicit TelemetryReader(Fact *parent);

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

    // datatypes for data signals
    using Field = TelemetryFileReader::Field;
    using Values = TelemetryFileReader::Values;

private:
    quint64 _recordID{};

    QList<Field> _fields;

    bool blockNotesChange;

    void addEventFact(quint64 time, const QString &name, const QString &value, const QString &uid);

private slots:
    void notesChanged();
    void updateStatus();

    //Database
    void setRecordInfo(quint64 id, QJsonObject info);

    void fileInfoLoaded(QJsonObject data);

    void dbResultsDataProc(quint64 telemetryID,
                           quint64 cacheID,
                           fieldData_t fieldData,
                           fieldNames_t fieldNames,
                           times_t times,
                           events_t events,
                           QGeoPath path,
                           Fact *f_events);

    void dbStatsFound(quint64 telemetryID, QVariantMap stats);
    void dbProgress(quint64 telemetryID, int v);

signals:
    // forwarded signals from file reader
    void rec_field(QString name, QString title, QString units);
    void rec_values(quint64 timestamp_ms, Values data, bool uplink);
    void rec_evt(quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink);
    void rec_msg(quint64 timestamp_ms, QString text, QString subsystem);
    void rec_meta(QString name, QJsonObject data, bool uplink);
    void rec_raw(quint64 timestamp_ms, uint16_t id, QByteArray data, bool uplink);

    //
    void parsingStarted();
    void parsingFinished();

    // called when file parsed and header info collected
    void recordInfoUpdated(quint64 id, QJsonObject data);

    void statsAvailable();
    void dataAvailable(quint64 cacheID);
    void recordFactTriggered(Fact *f);

public slots:
    void loadRecord(quint64 id);

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
