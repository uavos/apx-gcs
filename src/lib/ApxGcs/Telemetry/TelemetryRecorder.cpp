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
#include "TelemetryRecorder.h"

#include <ApxApp.h>
#include <ApxLog.h>
#include <Database/Database.h>
#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Vehicles/Vehicle.h>

#include <Nodes/Nodes.h>
//=============================================================================
TelemetryRecorder::TelemetryRecorder(Vehicle *vehicle, Fact *parent)
    : Fact(parent, "recorder", tr("Record"), tr("Enable telemetry recording"), Bool)
    , vehicle(vehicle)
    , v_dl_timestamp("dl_timestamp", vehicle)
    , dl_timestamp_s(0)
    , dl_timestamp_t0(0)
    , m_time(0)
{
    setIcon("record-rec");
    setValue(false);

    //vehicle forwarded recording signals
    connect(vehicle, &Vehicle::recordDownlink, this, &TelemetryRecorder::recordDownlink);
    connect(vehicle, &Vehicle::recordUplink, this, &TelemetryRecorder::recordUplink);
    connect(vehicle, &Vehicle::recordMessage, this, &TelemetryRecorder::recordMessage);
    connect(vehicle, &Vehicle::recordConfigUpdate, this, &TelemetryRecorder::recordConfigUpdate);
    connect(vehicle, &Vehicle::recordSerialData, this, &TelemetryRecorder::recordSerialData);

    //write config on each update
    connect(this->vehicle->f_nodes->storage,
            &NodesStorage::configInfoUpdated,
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

    connect(this, &Fact::valueChanged, this, &TelemetryRecorder::updateRecording);

    connect(this, &TelemetryRecorder::timeChanged, this, &TelemetryRecorder::updateStatus);

    recTelemetryID = 0;
    reqNewRecord = nullptr;

    recTrigger = false;
    timeUpdateTimer.setSingleShot(true);
    timeUpdateTimer.setInterval(500);
    connect(&timeUpdateTimer, &QTimer::timeout, this, &TelemetryRecorder::timeUpdate);

    recStopTimer.setSingleShot(true);
    connect(&recStopTimer, &QTimer::timeout, this, [=]() { setValue(false); });

    updateStatus();
}
//=============================================================================
void TelemetryRecorder::updateStatus()
{
    setStatus(AppRoot::timeToString(time(), true));
}
void TelemetryRecorder::updateRecording()
{
    recStopTimer.stop();
    reset();
    if (!recording()) {
        recTrigger = false;
    }
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
    timestamp = static_cast<quint64>(QDateTime::currentDateTime().toMSecsSinceEpoch());
    uplinkTime.start();
    //register telemetry file record
    if (!reqNewRecord) {
        QString title = vehicle->confTitle();
        if (!title.isEmpty())
            confTitle = title;
        DBReqTelemetryNewRecord *req
            = new DBReqTelemetryNewRecord(vehicle->uid,
                                          vehicle->callsign(),
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
    recTelemetryID = telemetryID;
    reqNewRecord = nullptr;
    configHash.clear();  //force write
    missionHash.clear(); //force write
    recordMission(false);
    recordConfig();
    recordDownlink();
    for (int i = 0; i < reqPendingList.size(); ++i) {
        DBReqTelemetryWriteBase *req = reqPendingList.at(i);
        req->telemetryID = recTelemetryID;
        req->exec();
    }
    reqPendingList.clear();
    //record shared info
    QVariantMap info;
    info["machineUID"] = ApxApp::machineUID();
    info["hostname"] = ApxApp::hostname();
    info["username"] = ApxApp::username();
    DBReqTelemetryWriteSharedInfo *req = new DBReqTelemetryWriteSharedInfo(telemetryID, info);
    req->exec();
}
//=============================================================================
quint64 TelemetryRecorder::getDataTimestamp()
{
    //snap to dl_timestamp
    if (!dl_timestamp_t0)
        dl_timestamp_t0 = v_dl_timestamp;
    uint t = v_dl_timestamp - dl_timestamp_t0;

    if (dl_timestamp_s != t) {
        dl_timestamp_s = t;
        uplinkTime.start();
    } else if (vehicle->streamType() != Vehicle::TELEMETRY)
        t += static_cast<uint>(uplinkTime.elapsed());
    setTime(t / 1000);
    return t;
}
//=============================================================================
void TelemetryRecorder::updateFactsMap()
{
    //fill facts map
    if (!factsMap.isEmpty())
        return;
    TelemetryDB::TelemetryFieldsMap map = Database::instance()->telemetry->fieldsMap();
    foreach (quint64 key, map.keys()) {
        Fact *f = vehicle->f_mandala->child(map.value(key));
        if (f)
            factsMap.insert(key, f);
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
    foreach (quint64 fieldID, factsMap.keys()) {
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
        } else
            recValues.append(v);
        values.append(QPair<quint64, double>(fieldID, v));
    }
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
void TelemetryRecorder::recordUplink(Fact *f)
{
    bool bID = dbCheckRecord();
    updateFactsMap();
    DBReqTelemetryWriteData *req = new DBReqTelemetryWriteData(recTelemetryID,
                                                               getDataTimestamp(),
                                                               factsMap.key(f),
                                                               f->value().toDouble(),
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
void TelemetryRecorder::recordMessage(QString nodeName, QString text, QString sn)
{
    writeEvent("msg", QString("[%1]%2").arg(nodeName).arg(text), sn, false);
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
    const QVariantMap &info = vehicle->f_nodes->storage->configInfo;
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
    if (vehicle->streamType() == Vehicle::TELEMETRY) {
        if (vehicle->flying()) {
            if (!recTrigger) {
                reset(); //restart
                setValue(true);
                recTrigger = true;
            }
        } else
            recTrigger = false;
        if (recording()) {
            if ((!recStopTimer.isActive()) && (!vehicle->flying()))
                recStopTimer.start(2000);
        }
    }
    if (!recording()) {
        return false;
    }
    return true;
}
//=============================================================================
bool TelemetryRecorder::recording() const
{
    return value().toBool();
}
void TelemetryRecorder::reset(void)
{
    recTelemetryID = 0;
    dl_timestamp_s = 0;
    dl_timestamp_t0 = 0;
    recValues.clear();
    setTime(0, true);
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
