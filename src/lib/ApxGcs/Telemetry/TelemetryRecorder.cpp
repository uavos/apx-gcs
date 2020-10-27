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
#include "TelemetryRecorder.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Vehicles/Vehicle.h>

#include <Nodes/Nodes.h>
//=============================================================================
TelemetryRecorder::TelemetryRecorder(Vehicle *vehicle, Fact *parent)
    : Fact(parent, "recorder", tr("Recorder"), tr("Telemetry recording"))
    , vehicle(vehicle)
{
    setIcon("record-rec");

    f_enable = new Fact(parent,
                        "enable",
                        tr("Record telemetry"),
                        tr("Enable telemetry recording"),
                        Bool);
    connect(f_enable, &Fact::valueChanged, this, &TelemetryRecorder::recordingChanged);
    connect(f_enable, &Fact::valueChanged, this, &TelemetryRecorder::restartRecording);

    //vehicle forwarded recording signals
    connect(vehicle, &Vehicle::recordDownlink, this, &TelemetryRecorder::recordDownlink);
    connect(vehicle, &Vehicle::recordUplink, this, &TelemetryRecorder::recordUplink);
    connect(vehicle, &Vehicle::recordNodeMessage, this, &TelemetryRecorder::recordNodeMessage);
    connect(vehicle, &Vehicle::recordConfigUpdate, this, &TelemetryRecorder::recordConfigUpdate);
    connect(vehicle, &Vehicle::recordSerialData, this, &TelemetryRecorder::recordSerialData);

    connect(vehicle, &Vehicle::recordConfig, this, &TelemetryRecorder::recordConfig);

    //write config on each update
    connect(vehicle->protocol(),
            &ProtocolVehicle::dbConfigInfoChanged,
            this,
            &TelemetryRecorder::recordConfig);

    //write mission on each upload or download
    connect(vehicle->f_mission, &VehicleMission::missionDownloaded, this, [this]() {
        connect(this->vehicle->f_mission->storage,
                &MissionStorage::saved,
                this,
                &TelemetryRecorder::recordMissionDownlink);
    });
    connect(vehicle->f_mission, &VehicleMission::missionUploaded, this, [this]() {
        connect(this->vehicle->f_mission->storage,
                &MissionStorage::saved,
                this,
                &TelemetryRecorder::recordMissionUplink);
    });

    connect(this, &TelemetryRecorder::timeChanged, this, &TelemetryRecorder::updateStatus);

    recTelemetryID = 0;
    reqNewRecord = nullptr;

    timeUpdateTimer.setSingleShot(true);
    timeUpdateTimer.setInterval(500);
    connect(&timeUpdateTimer, &QTimer::timeout, this, &TelemetryRecorder::timeUpdate);

    // auto recorder
    flightState_s = Vehicle::FS_UNKNOWN;
    recStopTimer.setSingleShot(true);
    connect(&recStopTimer, &QTimer::timeout, this, [this]() { setRecording(false); });

    //invalidate record ID after trash empty
    connect(Database::instance()->telemetry,
            &TelemetryDB::invalidateRecords,
            this,
            &TelemetryRecorder::restartRecording);

    updateStatus();
}
//=============================================================================
void TelemetryRecorder::updateStatus()
{
    setValue(AppRoot::timeToString(time(), true));
}
void TelemetryRecorder::restartRecording()
{
    recStopTimer.stop();
    reset();
}
//=============================================================================
void TelemetryRecorder::invalidateCache()
{
    Database::instance()->telemetry->markCacheInvalid(recTelemetryID);
}
//=============================================================================
//=============================================================================
bool TelemetryRecorder::dbCheckRecord()
{
    checkAutoRecord();
    if (recTelemetryID)
        return true;

    //register telemetry file record
    if (!reqNewRecord) {
        QString title = vehicle->confTitle();
        if (!title.isEmpty())
            confTitle = title;
        DBReqTelemetryNewRecord *req
            = new DBReqTelemetryNewRecord(vehicle->protocol()->uid(),
                                          vehicle->title(),
                                          confTitle,
                                          recording(),
                                          QDateTime::currentDateTime().toMSecsSinceEpoch());
        connect(req,
                &DBReqTelemetryNewRecord::idUpdated,
                this,
                &TelemetryRecorder::updateCurrentID,
                Qt::QueuedConnection);
        req->exec();
        reqNewRecord = req;
        reqPendingList.clear();
        apxConsole() << tr("Telemetry record request");
    }
    if (reqPendingList.size() > 50000)
        reqPendingList.takeFirst();
    return false;
}
void TelemetryRecorder::updateCurrentID(quint64 telemetryID)
{
    if (!reqNewRecord)
        return;
    //qDebug() << telemetryID << reqPendingList.size();
    recTelemetryID = telemetryID;
    reqNewRecord = nullptr;
    configHash.clear();  //force write
    missionHash.clear(); //force write
    recordMission(false);
    recordConfig();
    recordDownlink();
    for (auto req : reqPendingList) {
        req->telemetryID = recTelemetryID;
        req->exec();
    }
    reqPendingList.clear();
    //record shared info
    QVariantMap info;
    info["machineUID"] = App::machineUID();
    info["hostname"] = App::hostname();
    info["username"] = App::username();
    DBReqTelemetryWriteSharedInfo *req = new DBReqTelemetryWriteSharedInfo(telemetryID, info);
    req->exec();
}
//=============================================================================
quint64 TelemetryRecorder::getDataTimestamp()
{
    quint64 vts = vehicle->f_mandala->timestamp();
    //if (vts == 0)
    //    reset();
    if (!dl_timestamp_t0 || vts < dl_timestamp_t0)
        dl_timestamp_t0 = vts;
    quint64 t = vts - dl_timestamp_t0;

    if (m_currentTimestamp > t)
        reset();

    setTime(t / 1000);
    m_currentTimestamp = t;
    return t;
}
//=============================================================================
void TelemetryRecorder::updateFactsMap()
{
    //fill facts map
    if (!factsMap.isEmpty())
        return;
    const TelemetryDB::TelemetryFieldsMap &map = Database::instance()->telemetry->fieldsMap();
    for (auto i : map.keys()) {
        Fact *f = vehicle->f_mandala->fact(map.value(i));
        if (f)
            factsMap.insert(i, f);
    }
}
//=============================================================================
void TelemetryRecorder::writeEvent(const QString &name,
                                   const QString &value,
                                   const QString &uid,
                                   bool uplink)
{
    bool bID = dbCheckRecord();
    DBReqTelemetryWriteEvent *req = new DBReqTelemetryWriteEvent(recTelemetryID,
                                                                 getDataTimestamp(),
                                                                 name,
                                                                 value,
                                                                 uid,
                                                                 uplink);
    if (bID) {
        req->exec();
        invalidateCache();
    } else
        reqPendingList.append(req);
}
//=============================================================================
//=============================================================================
void TelemetryRecorder::recordDownlink()
{
    //write all updated facts
    bool bID = dbCheckRecord();
    quint64 t = getDataTimestamp();
    //collect changed facts
    QList<QPair<quint64, double>> values;
    int iv = -1;
    updateFactsMap();
    for (auto fieldID : factsMap.keys()) {
        Fact *f = factsMap.value(fieldID);
        iv++;
        QVariant vv = f->value();
        double v = vv.toDouble();
        if (iv < recValues.size()) {
            if (recValues.at(iv) == v) {
                continue;
            } else {
                recValues[iv] = v;
            }
        } else {
            recValues.append(v);
            if (v == 0)
                continue;
        }
        values.append(QPair<quint64, double>(fieldID, v));
    }
    //qDebug() << values.size();
    if (values.isEmpty())
        return;
    for (int i = 0; i < values.size(); ++i) {
        quint64 fieldID = values.at(i).first;
        double v = values.at(i).second;
        DBReqTelemetryWriteData *req = new DBReqTelemetryWriteData(recTelemetryID,
                                                                   t,
                                                                   fieldID,
                                                                   v,
                                                                   false);
        if (bID) {
            req->exec();
            invalidateCache();
        } else
            reqPendingList.append(req);
    }
}
//=============================================================================
void TelemetryRecorder::recordUplink(xbus::pid_s pid, QVariant value)
{
    bool bID = dbCheckRecord();
    updateFactsMap();

    Fact *f = vehicle->f_mandala->fact(pid.uid);
    if (!f)
        return;
    DBReqTelemetryWriteData *req = new DBReqTelemetryWriteData(recTelemetryID,
                                                               getDataTimestamp(),
                                                               factsMap.key(f),
                                                               value.toDouble(),
                                                               true);
    if (bID) {
        req->exec();
        invalidateCache();
    } else
        reqPendingList.append(req);
}
//=============================================================================
// write data slots
//=============================================================================
void TelemetryRecorder::recordNodeMessage(QString subsystem, QString text, QString sn)
{
    writeEvent("msg", QString("[%1]%2").arg(subsystem).arg(text), sn, false);
}
void TelemetryRecorder::recordConfigUpdate(QString nodeName,
                                           QString fieldName,
                                           QString value,
                                           QString sn)
{
    writeEvent("conf", QString("%1/%2=%3").arg(nodeName).arg(fieldName).arg(value), sn, true);
}
void TelemetryRecorder::recordSerialData(quint16 portNo, QByteArray data, bool uplink)
{
    writeEvent("serial", data.toHex().toUpper(), QString::number(portNo), uplink);
}
//=============================================================================
void TelemetryRecorder::recordMissionDownlink()
{
    disconnect(vehicle->f_mission->storage,
               &MissionStorage::saved,
               this,
               &TelemetryRecorder::recordMissionDownlink);
    recordMission(false);
}
void TelemetryRecorder::recordMissionUplink()
{
    disconnect(vehicle->f_mission->storage,
               &MissionStorage::saved,
               this,
               &TelemetryRecorder::recordMissionUplink);
    recordMission(true);
}
void TelemetryRecorder::recordMission(bool uplink)
{
    QString title = vehicle->f_mission->f_title->text();
    QString hash = vehicle->f_mission->storage->dbHash;
    if (hash.isEmpty())
        return;
    if (missionHash == hash)
        return;
    missionHash = hash;
    //qDebug()<<title<<hash;
    writeEvent("mission", title, hash, uplink);
}
void TelemetryRecorder::recordConfig()
{
    if (!dbCheckRecord())
        return;

    const QVariantMap &info = vehicle->protocol()->dbConfigInfo();
    QString hash = info.value("hash").toString();
    if (hash.isEmpty())
        return;
    if (configHash == hash)
        return;
    configHash = hash;
    //qDebug()<<hash<<info;
    QString title = info.value("title").toString();
    writeEvent("nodes", title, hash, false);

    //check for config title change
    if (confTitle != title) {
        confTitle = title;
        if (recTelemetryID) {
            QVariantMap info;
            info.insert("comment", title);
            DBReqTelemetryWriteInfo *req = new DBReqTelemetryWriteInfo(recTelemetryID, info);
            req->exec();
        }
    }
}
//=============================================================================
//=============================================================================

//=============================================================================
bool TelemetryRecorder::checkAutoRecord(void)
{
    if (vehicle->protocol()->streamType() != ProtocolVehicle::TELEMETRY)
        return recording();

    Vehicle::FlightState fs = vehicle->flightState();
    if (flightState_s != fs) {
        flightState_s = fs; //only once

        // stop recording when landed
        if (fs == Vehicle::FS_LANDED && recording() && !recStopTimer.isActive())
            recStopTimer.start(2000);

        // start or restart when starts flying
        if (fs == Vehicle::FS_TAKEOFF) {
            reset(); //restart
            setRecording(true);
        }
    }
    return recording();
}
//=============================================================================
bool TelemetryRecorder::recording() const
{
    return f_enable->value().toBool();
}
void TelemetryRecorder::setRecording(bool v)
{
    f_enable->setValue(v);
}
void TelemetryRecorder::reset(void)
{
    recTelemetryID = 0;
    dl_timestamp_t0 = 0;
    recValues.clear();
    setTime(0, true);
}
//=============================================================================
quint64 TelemetryRecorder::currentTimstamp() const
{
    return m_currentTimestamp;
}
//=============================================================================
quint64 TelemetryRecorder::time() const
{
    return m_time;
}
void TelemetryRecorder::setTime(quint64 v, bool forceUpdate)
{
    if (m_time == v)
        return;
    m_time = v;
    if (forceUpdate) {
        timeUpdateTimer.stop();
        timeUpdate();
        return;
    }
    if (!timeUpdateTimer.isActive())
        timeUpdateTimer.start();
}
void TelemetryRecorder::timeUpdate(void)
{
    emit timeChanged();
}
//=============================================================================
