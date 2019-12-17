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
#ifndef TelemetryRecorder_H
#define TelemetryRecorder_H
#include <Database/TelemetryReqWrite.h>
#include <Fact/Fact.h>
#include <Vehicles/VehicleMandalaValue.h>
#include <Vehicles/Vehicles.h>
#include <QtCore>
class Recorder;
//=============================================================================
class TelemetryRecorder : public Fact
{
    Q_OBJECT
public:
    explicit TelemetryRecorder(Vehicle *vehicle, Fact *parent);

    quint64 currentTimstamp() const;

private:
    Vehicle *vehicle;

    //database
    bool dbCheckRecord();

    quint64 recTelemetryID;
    QList<double> recValues;
    QHash<quint64, Fact *> factsMap;

    void updateFactsMap();

    //auto recorder
    bool checkAutoRecord(void);
    Vehicle::FlightState flightState_s;
    QTimer timeUpdateTimer, recStopTimer;

    //timestamp
    VehicleMandalaValue<uint> v_dl_timestamp;
    uint dl_timestamp_s, dl_timestamp_t0;
    QElapsedTimer uplinkTime;

    quint64 timestamp;
    quint64 getDataTimestamp();

    DatabaseRequest *reqNewRecord;
    QList<DBReqTelemetryWriteBase *> reqPendingList;

    QString confTitle;

    //cache duplicates
    QString configHash;
    QString missionHash;

    void invalidateCache();

    quint64 m_currentTimestamp;

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
    void recordUplink(Fact *f);
    //events
    void recordNodeMessage(QString nodeName, QString text, QString sn);
    void recordConfigUpdate(QString nodeName, QString fieldName, QString value, QString sn);
    void recordSerialData(quint16 portNo, QByteArray data, bool uplink);

    //PROPERTIES
public:
    bool recording() const;

    Q_PROPERTY(quint64 time READ time NOTIFY timeChanged)
    quint64 time() const;

private:
    quint64 m_time;
    void setTime(quint64 v, bool forceUpdate = false);
signals:
    void timeChanged();
public slots:
    void reset(void);
};
//=============================================================================
#endif
