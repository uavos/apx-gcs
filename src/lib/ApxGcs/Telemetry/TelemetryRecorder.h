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
#ifndef TelemetryRecorder_H
#define TelemetryRecorder_H
#include <Database/TelemetryReqWrite.h>
#include <Fact/Fact.h>
#include <Vehicles/Vehicles.h>
#include <QtCore>
class Recorder;
//=============================================================================
class TelemetryRecorder : public Fact
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)

public:
    explicit TelemetryRecorder(Vehicle *vehicle, Fact *parent);

    quint64 currentTimstamp() const;

private:
    Vehicle *vehicle;

    Fact *f_enable;

    //database
    bool dbCheckRecord();

    quint64 recTelemetryID{};
    QList<double> recValues;
    QHash<quint64, Fact *> factsMap;

    void updateFactsMap();

    //auto recorder
    bool checkAutoRecord(void);
    Vehicle::FlightState flightState_s;
    QTimer timeUpdateTimer, recStopTimer;

    //timestamp
    quint64 dl_timestamp_t0{0};

    quint64 getDataTimestamp();

    DatabaseRequest *reqNewRecord;
    QList<DBReqTelemetryWriteBase *> reqPendingList;

    QString confTitle;

    //cache duplicates
    QString configHash;
    QString missionHash;

    void invalidateCache();

    quint64 m_currentTimestamp{0};

private slots:
    void updateStatus();
    void timeUpdate(void);
    void restartRecording();

    //database
    void updateCurrentID(quint64 telemetryID);
    void writeEvent(const QString &name, const QString &value, const QString &uid, bool uplink);

    //internal flow
    void recordMissionDownlink();
    void recordMissionUplink();
    void recordMission(bool uplink);
    void recordConfig();

public slots:
    //exported slots for recording
    void recordDownlink();
    void recordUplink(xbus::pid_s pid, QVariant value);
    //events
    void recordNodeMessage(QString subsystem, QString text, QString sn);
    void recordConfigUpdate(QString nodeName, QString fieldName, QString value, QString sn);
    void recordSerialData(quint16 portNo, QByteArray data, bool uplink);

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
//=============================================================================
#endif
