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

    connect(vehicle, &Vehicle::messageReported, this, &TelemetryRecorder::recordMsg);

    // record mission on each upload or download
    connect(vehicle->f_mission, &VehicleMission::missionDownloaded, this, [this]() {
        recordMission(false);
    });
    connect(vehicle->f_mission, &VehicleMission::missionUploaded, this, [this]() {
        recordMission(true);
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

    connect(App::instance(), &App::appQuit, this, [this]() { disconnect(); });
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

bool TelemetryRecorder::dbCheckRecord()
{
    checkAutoRecord();

    if (_file.isOpen())
        return true;

    auto time_utc = QDateTime::currentDateTime().toMSecsSinceEpoch();

    apxConsole() << tr("Telemetry record request");

    // construct new file name

    auto dir = AppDirs::telemetry();
    if (!recording())
        dir.setPath(dir.absoluteFilePath("trash"));
    dir.mkpath(".");

    QStringList st;
    st.append(QString::number(time_utc, 16).toUpper());

    // st.append(t.toString("yyMMddHHmm"));

    QString callsign = _vehicle->title();
    if (callsign.isEmpty())
        callsign = _vehicle->confTitle();
    if (callsign.isEmpty())
        callsign = "U";

    st.append(callsign);

    QString fname;
    for (int i = 0; i < 100; ++i) {
        QString s = st.join('_');
        if (i > 0)
            s.append(QString("_%1").arg(i, 2, 10, QChar('0')));

        s.append('.').append(telemetry::APXTLM_FTYPE);

        if (!QFile::exists(dir.absoluteFilePath(s))) {
            fname = s;
            break;
        }
    }
    if (fname.isEmpty()) {
        qWarning() << "failed to create file name";
        return false;
    }
    fname = dir.absoluteFilePath(fname);

    _file.create(fname, time_utc, _vehicle);

    // record initial meta data
    recordConfig();
    recordMission(false);

    return false;
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
    setTime(t / 1000);

    _file.write_values(t, values, false);
}
void TelemetryRecorder::recordData(PBase::Values values, bool uplink)
{
    // qDebug() << values << uplink;

    dbCheckRecord();

    _file.write_values(getEventTimestamp(), values, uplink);
}

// write data slots

void TelemetryRecorder::recordNotification(QString msg,
                                           QString subsystem,
                                           AppNotify::NotifyFlags flags,
                                           Fact *fact)
{
    if (msg.isEmpty())
        return;
    if (flags & AppNotify::FromVehicle)
        return;

    _file.write_msg(getEventTimestamp(), msg, "gcs/" + subsystem);
}

void TelemetryRecorder::recordMsg(QString msg, QString subsystem)
{
    if (msg.isEmpty())
        return;
    _file.write_msg(getEventTimestamp(), msg, subsystem);
}

void TelemetryRecorder::recordConfigUpdate(NodeItem *node, QString name, QString value)
{
    value = QString("%1/%2=%3").arg(node->title()).arg(name).arg(value);

    _file.write_evt(getEventTimestamp(), "conf", value, node->uid(), true);
}
void TelemetryRecorder::recordSerialData(quint16 portNo, QByteArray data, bool uplink)
{
    _file.write_raw(getEventTimestamp(), portNo, data, uplink);
}

void TelemetryRecorder::recordMission(bool uplink)
{
    _file.write_meta("mission", _vehicle->f_mission->toJsonDocument().object(), uplink);
}
void TelemetryRecorder::recordConfig()
{
    _file.write_meta("nodes", _vehicle->toJsonDocument().object(), false);
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
    _file.close();

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
