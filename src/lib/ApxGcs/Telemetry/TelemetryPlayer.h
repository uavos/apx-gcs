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
#include <QtCore>

class Vehicle;
class Telemetry;

class TelemetryPlayer : public Fact
{
    Q_OBJECT
public:
    explicit TelemetryPlayer(Telemetry *telemetry, Fact *parent);

    Fact *f_record;
    Fact *f_filter;
    Fact *f_time;
    Fact *f_speed;

    Fact *f_play;
    Fact *f_stop;
    Fact *f_rewind;

private:
    Telemetry *telemetry;
    Vehicle *vehicle;

    quint64 cacheID;

    QTimer timer;
    quint64 playTime0;
    QElapsedTimer playTime;
    quint64 tNext;

    quint64 setTime0;
    double _speed;
    double _time;

    bool blockTimeChange;

    QHash<quint64, Fact *> factsMap;
    DatabaseRequest::Records events;
    int iEventRec;

    QHash<quint64, int> dataPosMap;
    double sampleValue(quint64 fieldID, double t);

    void loadConfValue(const QString &sn, QString s);

private slots:
    void updateActions();
    void updateStatus();
    void updateSpeed();
    void updateActive();
    void updateTime();

    void reset();
    void setCacheId(quint64 v);

    void next();

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
