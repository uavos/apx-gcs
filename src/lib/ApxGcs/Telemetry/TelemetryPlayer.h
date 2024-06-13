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

#include <Database/DatabaseRequest.h>
#include <Fact/Fact.h>

#include "TelemetryReader.h"

class Vehicle;
class TelemetryReader;

class TelemetryPlayer : public Fact
{
    Q_OBJECT
public:
    explicit TelemetryPlayer(TelemetryReader *reader, Fact *parent);

    Fact *f_record;
    Fact *f_filter;
    Fact *f_time;
    Fact *f_speed;

    Fact *f_play;
    Fact *f_stop;
    Fact *f_rewind;

private:
    TelemetryReader *reader;
    Vehicle *vehicle;

    TelemetryFileReader _file;
    QHash<int, MandalaFact *> _fieldsMap;

    QTimer timer;
    quint64 playTime0;
    QElapsedTimer playTime;
    quint64 tNext;

    quint64 setTime0;
    double _speed;
    double _time;

    bool blockTimeChange;

    QHash<quint64, Fact *> factByDBID;
    DatabaseRequest::Records events;
    int iEventRec;

    QHash<quint64, int> dataPosMap;
    double sampleValue(quint64 fieldID, double t);

    void loadConfValue(const QString &sn, QString s);

    struct Index
    {
        quint64 timestamp_ms;
        quint64 offset;
        TelemetryReader::Values values;
    };
    QList<Index> _index;

private slots:
    void updateActions();
    void updateStatus();
    void updateSpeed();
    void updateActive();
    void updateTime();

    void reset();

    void next();

    // file reader
    void rec_started();
    void rec_finished();
    void rec_index(quint64 timestamp_ms, quint64 offset, TelemetryReader::Values values);

    //database
    void dbRequestEvents(quint64 t);
    void eventsLoaded(DatabaseRequest::Records records);
    void missionDataLoaded(QString value, QString uid, bool uplink);
    void nodesDataLoaded(QString value, QString uid, bool uplink);
    void nodesConfUpdatesLoaded(DatabaseRequest::Records records);

    void play();
    void stop();
    void rewind();

signals:
    void discardRequests();
};
