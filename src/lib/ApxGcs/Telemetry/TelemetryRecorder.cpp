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
#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
#include <Vehicles/Vehicle.h>

TelemetryRecorder::TelemetryRecorder(Vehicle *vehicle, Fact *parent)
    : Fact(parent, "recorder", tr("Recorder"), tr("Telemetry recording"))
    , _vehicle(vehicle)
    , _stream(prepareFieldsMap())
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

void TelemetryRecorder::checkFileRecord()
{
    checkAutoRecord();

    if (_stream_file.isOpen())
        return;

    apxConsole() << tr("Telemetry record request");

    // construct new file name

    auto timestamp = QDateTime::currentDateTime();

    QString unitName = _vehicle->title();
    if (unitName.isEmpty())
        unitName = _vehicle->confTitle();

    const auto info = prepareFileInfo();
    const auto time_utc = timestamp.toMSecsSinceEpoch();

    // create new file
    const auto basename = db::storage::Session::telemetryFileBasename(timestamp, unitName);
    const auto filePath = db::storage::Session::telemetryFilePathUnique(basename);
    if (filePath.isEmpty())
        return;

    _stream_file.setFileName(filePath);
    if (!_stream_file.open(QIODevice::WriteOnly)) {
        apxMsgW() << tr("Failed to open file").append(':') << filePath;
        return;
    }
    _stream.init(&_stream_file, basename, time_utc, info);

    // record initial mandala state
    for (auto f : _vehicle->f_mandala->valueFacts()) {
        if (!(f->everReceived() || f->everSent()))
            continue;
        _stream.write_value(f->uid(), f->value());
    }

    // record initial meta data
    if (_vehicle->f_nodes->valid()) {
        auto hash = _configHash;
        _configHash.clear();
        recordConfig(hash, _vehicle->confTitle());
    }

    if (!_vehicle->f_mission->empty())
        recordMission(false);

    // create DB record
    auto req = new db::storage::TelemetryCreateRecord(time_utc,
                                                      QFileInfo(filePath).completeBaseName(),
                                                      info,
                                                      !recording());
    req->exec();
}

QJsonObject TelemetryRecorder::prepareFileInfo()
{
    QJsonObject info;

    info["unit"] = _vehicle->get_info();
    info["conf"] = _vehicle->confTitle();

    QJsonObject sw;
    sw["version"] = QCoreApplication::applicationVersion();
    sw["hash"] = App::git_hash();
    info["sw"] = sw;

    QJsonObject host;
    host["hostname"] = App::hostname();
    host["username"] = App::username();
    host["uid"] = App::machineUID();
    info["host"] = host;

    return info;
}

TelemetryFileWriter::Fields TelemetryRecorder::prepareFieldsMap()
{
    TelemetryFileWriter::Fields fields;
    for (auto f : _vehicle->f_mandala->valueFacts()) {
        // guess field storage format
        telemetry::dspec_e dspec;
        do {
            if (f->dataType() != Fact::Float) {
                // recorder will truncate uint size if necessary
                dspec = telemetry::dspec_e::u32;
                break;
            }
            // guess float types
            if (f->is_gps_converted()) {
                dspec = telemetry::dspec_e::a32;
                break;
            }
            switch (f->fmt().fmt) {
            default:
                dspec = telemetry::dspec_e::f32;
                break;
            case mandala::fmt_s16_rad:
            case mandala::fmt_s16_rad2:
                dspec = telemetry::dspec_e::a16;
                break;
            case mandala::fmt_f16:
            case mandala::fmt_s8:
            case mandala::fmt_s8_10:
            case mandala::fmt_u8_10:
            case mandala::fmt_s8_01:
            case mandala::fmt_s8_001:
            case mandala::fmt_u8_01:
            case mandala::fmt_u8_001:
            case mandala::fmt_u8_u:
            case mandala::fmt_s8_u:
            case mandala::fmt_s8_rad:
                dspec = telemetry::dspec_e::f16;
                break;
            }
        } while (0);
        // prepare field map for telemetry file writer
        fields[f->uid()] = {f->mpath(), f->title(), f->units(), dspec};
    }
    return fields;
}

quint64 TelemetryRecorder::getEventTimestamp()
{
    if (!_tsElapsed.isValid()) {
        _tsElapsed.start();
        qDebug() << _vehicle->title() << "0 PRE";
        return 0;
    }

    quint64 elapsed = _tsElapsed.elapsed();
    quint64 t = elapsed + _ts_t2;

    // qDebug() << _vehicle->title() << t << elapsed << _ts_t2;
    return t;
}

void TelemetryRecorder::recordTelemetry(PBase::Values values, quint64 timestamp_ms)
{
    // always start DB recorder time counter from zero
    auto t = timestamp_ms;

    if (t < _ts_t0)
        reset();

    checkFileRecord();

    if (_reset_timestamp) {
        qDebug() << _vehicle->title() << "reset timestamp:" << t;
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

    // qDebug() << _vehicle->title() << t << values.size();
    _stream.write_values(t, values, false);
}
void TelemetryRecorder::recordData(PBase::Values values, bool uplink)
{
    // qDebug() << values << uplink;

    checkFileRecord();

    _stream.write_values(getEventTimestamp(), values, uplink);
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

    _stream.write_msg(getEventTimestamp(), msg, "gcs/" + subsystem);
}

void TelemetryRecorder::recordMsg(QString msg, QString subsystem)
{
    if (msg.isEmpty())
        return;
    _stream.write_msg(getEventTimestamp(), msg, subsystem);
}

void TelemetryRecorder::recordConfigUpdate(NodeItem *node, QString name, QString value)
{
    value = QString("%1/%2=%3").arg(node->title()).arg(name).arg(value);

    _stream.write_evt(getEventTimestamp(), "conf", value, node->uid(), true);
}
void TelemetryRecorder::recordSerialData(quint16 portNo, QByteArray data, bool uplink)
{
    _stream.write_raw(getEventTimestamp(), portNo, data, uplink);
}

void TelemetryRecorder::recordMission(bool uplink)
{
    const auto &data = _vehicle->f_mission->toJsonDocument().object();
    _stream.write_evt(getEventTimestamp(),
                      "mission",
                      data["title"].toString(),
                      data["hash"].toString(),
                      uplink);
    _stream.write_meta("mission", data, uplink);
}
void TelemetryRecorder::recordConfig(QString hash, QString title)
{
    if (hash.isEmpty())
        return;
    if (_configHash == hash)
        return;
    _configHash = hash;
    _stream.write_evt(getEventTimestamp(), "nodes", title, hash, false);
    _stream.write_meta("nodes", _vehicle->toJsonDocument().object(), false);
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
    _stream.close();

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
