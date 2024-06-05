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

#include <Database/TelemetryReqWrite.h>
#include <Fact/Fact.h>
#include <Vehicles/Vehicles.h>
#include <QtCore>

#include "TelemetryFileWriter.h"

class Recorder;
class NodeItem;

class TelemetryRecorder : public Fact
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)

public:
    explicit TelemetryRecorder(Vehicle *vehicle, Fact *parent);

private:
    Vehicle *_vehicle;

    Fact *f_enable;

    TelemetryFileWriter _file;

    //data file
    void checkFileRecord();

    //auto recorder
    bool checkAutoRecord(void);
    Vehicle::FlightState flightState_s;
    QTimer timeUpdateTimer, recStopTimer;

    //timestamp
    bool _reset_timestamp{true};
    quint64 _ts_t0{}; // telemetry record start time
    quint64 _ts_t1{}; // telemetry shift time (data in beginning)
    quint64 _ts_t2{}; // current telemetry corrected timestamp
    QElapsedTimer _tsElapsed;

    quint64 getEventTimestamp();

    QString confTitle;

private slots:
    void updateStatus();
    void timeUpdate(void);
    void restartRecording();

    //internal flow
    void recordMission(bool uplink);
    void recordConfig();

public slots:
    //exported slots for recording
    void recordTelemetry(PBase::Values values, quint64 timestamp_ms);
    void recordData(PBase::Values values, bool uplink);

    //events
    void recordConfigUpdate(NodeItem *node, QString name, QString value);
    void recordSerialData(quint16 portNo, QByteArray data, bool uplink);
    void recordNotification(QString msg,
                            QString subsystem,
                            AppNotify::NotifyFlags flags,
                            Fact *fact);
    void recordMsg(QString msg, QString subsystem);

    //PROPERTIES
public:
    bool recording() const;
    void setRecording(bool v);

    Q_PROPERTY(quint64 time READ time NOTIFY timeChanged)
    quint64 time() const;

private:
    quint64 m_time{0};
    void setTime(quint64 v, bool forceUpdate = false);

signals:
    void timeChanged();
    void recordingChanged();

public slots:
    void reset(void);
};
