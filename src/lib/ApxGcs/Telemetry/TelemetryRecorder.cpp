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

TelemetryRecorder::TelemetryRecorder(Vehicle *vehicle, Fact *parent)
    : Fact(parent, "recorder", tr("Recorder"), tr("Telemetry recording"))
    , _vehicle(vehicle)
{
    setIcon("record-rec");

    f_enable = new Fact(parent,
                        "enable",
                        tr("Record telemetry"),
                        tr("Enable telemetry recording"),
                        Bool);
    connect(f_enable, &Fact::valueChanged, this, &TelemetryRecorder::recordingChanged);
    connect(f_enable, &Fact::valueChanged, this, &TelemetryRecorder::restartRecording);

    // record doenlink/uplink
    connect(vehicle->f_mandala,
            &Mandala::recordTelemetry,
            this,
            &TelemetryRecorder::recordTelemetry);
    connect(vehicle->f_mandala, &Mandala::recordData, this, &TelemetryRecorder::recordData);

    // record serial port data
    connect(vehicle->protocol()->data(),
            &PData::serialData,
            this,
            [this](quint8 portID, QByteArray data) { recordSerialData(portID, data, false); });
    connect(vehicle->protocol()->data(),
            &PData::sendSerial,
            this,
            [this](quint8 portID, QByteArray data) { recordSerialData(portID, data, true); });

    // record config on each upload or save
    connect(vehicle->storage(),
            &VehicleStorage::configSaved,
            this,
            &TelemetryRecorder::recordConfig);

    // record text messages
    connect(vehicle->f_nodes,
            &Nodes::fieldUploadReport,
            this,
            &TelemetryRecorder::recordConfigUpdate);

    connect(AppNotify::instance(),
            &AppNotify::notification,
            this,
            &TelemetryRecorder::recordNotification);

    // record mission on each upload or download
    connect(vehicle->f_mission, &VehicleMission::missionDownloaded, this, [this]() {
        connect(this->_vehicle->f_mission->storage,
                &MissionStorage::saved,
                this,
                &TelemetryRecorder::recordMissionDownlink);
    });
    connect(vehicle->f_mission, &VehicleMission::missionUploaded, this, [this]() {
        connect(this->_vehicle->f_mission->storage,
                &MissionStorage::saved,
                this,
                &TelemetryRecorder::recordMissionUplink);
    });

    // display
    connect(this, &TelemetryRecorder::timeChanged, this, &TelemetryRecorder::updateStatus);

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

void TelemetryRecorder::updateStatus()
{
    setValue(AppRoot::timeToString(time(), true));
}
void TelemetryRecorder::restartRecording()
{
    recStopTimer.stop();
    reset();
}

void TelemetryRecorder::invalidateCache()
{
    if (!recTelemetryID)
        return;
    Database::instance()->telemetry->markCacheInvalid(recTelemetryID);
}

bool TelemetryRecorder::dbCheckRecord()
{
    checkAutoRecord();
    if (recTelemetryID)
        return true;

    //register telemetry file record
    if (!reqNewRecord) {
        QString title = _vehicle->confTitle();
        if (!title.isEmpty())
            confTitle = title;
        DBReqTelemetryNewRecord *req
            = new DBReqTelemetryNewRecord(_vehicle->uid(),
                                          _vehicle->title(),
                                          confTitle,
                                          recording(),
                                          QDateTime::currentDateTime().toMSecsSinceEpoch());
        connect(req,
                &DBReqTelemetryNewRecord::idUpdated,
                this,
                &TelemetryRecorder::dbRecordCreated,
                Qt::QueuedConnection);
        reqNewRecord = req;
        reqPendingList.clear();

        // write initial mandala state
        PBase::Values values;
        for (auto f : _vehicle->f_mandala->valueFacts()) {
            if (!f->everReceived())
                continue;
            values.insert(f->uid(), f->getValueForStream());
        }
        _values = values;
        reqPendingList.append(new DBReqTelemetryWriteData(0, 0, _values, false));

        apxConsole() << tr("Telemetry record request");
        reqNewRecord->exec();
    }

    if (reqPendingList.size() > 50000)
        reqPendingList.takeFirst();

    return false;
}
void TelemetryRecorder::dbRecordCreated(quint64 telemetryID)
{
    if (!reqNewRecord)
        return;
    if (!telemetryID)
        return;

    //qDebug() << telemetryID << reqPendingList.size();
    auto chash = configHash;
    recTelemetryID = telemetryID;
    reqNewRecord = nullptr;
    configHash.clear();  //force write
    missionHash.clear(); //force write

    recordMission(false);
    recordConfig(chash, "init");

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
void TelemetryRecorder::cleanupValues(PBase::Values *values)
{
    for (auto uid : values->keys()) {
        auto v = values->value(uid);
        if (_values.value(uid) == v) {
            values->remove(uid);
        } else {
            _values.insert(uid, v);
        }
    }
}
void TelemetryRecorder::dbWriteRequest(DBReqTelemetryWriteBase *req)
{
    if (recTelemetryID) {
        req->exec();
        invalidateCache();
    } else {
        reqPendingList.append(req);
    }
}
quint64 TelemetryRecorder::getEventTimestamp()
{
    if (!_tsElapsed.isValid()) {
        _tsElapsed.start();
        return 0;
    }

    quint64 t = _tsElapsed.elapsed();
    t += _ts_t2;
    return t;
}

void TelemetryRecorder::recordTelemetry(PBase::Values values, quint64 timestamp_ms)
{
    // always start DB recorder time counter from zero
    auto t = timestamp_ms;

    if (t < _ts_t0)
        reset();

    dbCheckRecord();

    if (_reset_timestamp) {
        _reset_timestamp = false;
        _ts_t0 = t;
        if (_tsElapsed.isValid()) {
            _ts_t1 = _tsElapsed.elapsed();
            _ts_t1 = (_ts_t1 / 100 + 1) * 100;
        }
    }
    t -= _ts_t0;
    t += _ts_t1;
    _ts_t2 = t;
    _tsElapsed.start();

    // qDebug() << _vehicle->title() << "ts" << t << _ts_t0;

    setTime(t / 1000);

    // record changed values only
    cleanupValues(&values);
    if (values.isEmpty())
        return;

    dbWriteRequest(new DBReqTelemetryWriteData(recTelemetryID, t, values, false));
}
void TelemetryRecorder::recordData(PBase::Values values, bool uplink)
{
    if (!uplink)
        cleanupValues(&values);
    if (values.isEmpty())
        return;

    dbCheckRecord();

    dbWriteRequest(new DBReqTelemetryWriteData(recTelemetryID, getEventTimestamp(), values, uplink));
}

void TelemetryRecorder::writeEvent(const QString &name,
                                   const QString &value,
                                   const QString &uid,
                                   bool uplink)
{
    dbCheckRecord();

    auto req = new DBReqTelemetryWriteEvent(recTelemetryID,
                                            getEventTimestamp(),
                                            name,
                                            value,
                                            uid,
                                            uplink);
    dbWriteRequest(req);
}

// write data slots

void TelemetryRecorder::recordNotification(QString msg,
                                           QString subsystem,
                                           AppNotify::NotifyFlags flags,
                                           Fact *fact)
{
    if (msg.isEmpty())
        return;
    QString uid;
    writeEvent("msg", QString("[%1]%2").arg(subsystem).arg(msg), uid, false);
}
void TelemetryRecorder::recordConfigUpdate(NodeItem *node, QString name, QString value)
{
    writeEvent("conf",
               QString("%1/%2=%3").arg(node->title()).arg(name).arg(value),
               node->uid(),
               true);
}
void TelemetryRecorder::recordSerialData(quint16 portNo, QByteArray data, bool uplink)
{
    writeEvent("serial", data.toHex().toUpper(), QString::number(portNo), uplink);
}

void TelemetryRecorder::recordMissionDownlink()
{
    disconnect(_vehicle->f_mission->storage,
               &MissionStorage::saved,
               this,
               &TelemetryRecorder::recordMissionDownlink);
    recordMission(false);
}
void TelemetryRecorder::recordMissionUplink()
{
    disconnect(_vehicle->f_mission->storage,
               &MissionStorage::saved,
               this,
               &TelemetryRecorder::recordMissionUplink);
    recordMission(true);
}
void TelemetryRecorder::recordMission(bool uplink)
{
    QString title = _vehicle->f_mission->f_title->text();
    QString hash = _vehicle->f_mission->storage->dbHash;
    if (hash.isEmpty())
        return;
    if (missionHash == hash)
        return;
    missionHash = hash;
    //qDebug()<<title<<hash;
    writeEvent("mission", title, hash, uplink);
}
void TelemetryRecorder::recordConfig(QString hash, QString title)
{
    if (!dbCheckRecord())
        return;

    if (hash.isEmpty())
        return;
    if (configHash == hash)
        return;
    configHash = hash;
    //qDebug()<<hash<<info;
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

bool TelemetryRecorder::checkAutoRecord(void)
{
    if (_vehicle->streamType() != PVehicle::TELEMETRY)
        return recording();

    Vehicle::FlightState fs = _vehicle->flightState();
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

    _reset_timestamp = true;
    _ts_t0 = _ts_t1 = _ts_t2 = 0;
    _tsElapsed.invalidate();
    qDebug() << "record reset";

    setTime(0, true);
}

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
