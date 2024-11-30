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
#include <Mandala/Mandala.h>

#include "TelemetryReader.h"

class Vehicle;
class TelemetryReader;

class TelemetryPlayer : public Fact
{
    Q_OBJECT
public:
    explicit TelemetryPlayer(TelemetryReader *reader, Vehicle *vehicle, Fact *parent);

    Fact *f_time;
    Fact *f_speed;

    Fact *f_play;
    Fact *f_stop;
    Fact *f_rewind;

private:
    TelemetryReader *reader;
    Vehicle *vehicle;

    QFile _stream_file;
    TelemetryFileReader _stream;

    std::vector<MandalaFact *> _facts_by_index;
    TelemetryReader::Values _values;

    QTimer timer;
    quint64 playTime0;
    QElapsedTimer playTime;
    quint64 tNext;

    quint64 setTime0;
    double _speed;
    quint64 _time;
    bool _values_init;

    bool blockTimeChange;

    void loadConfValue(QString uid, QString param, QString value);
    void loadLatestMeta(Fact *group, quint64 time);

    MandalaFact *updateMandalaFact(size_t index, const QVariant &value);
    void message(QString msg, bool uplink, QString subsystem);

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

    void rec_field(TelemetryReader::Field field);
    void rec_values(quint64 timestamp_ms, TelemetryReader::Values data, bool uplink);
    void rec_evt(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink);
    void rec_jso(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink);

    void play();
    void stop();
    void rewind();

signals:
    void discardRequests();
};
