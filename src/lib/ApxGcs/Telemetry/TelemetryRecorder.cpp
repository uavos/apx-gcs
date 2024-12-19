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
#include <Fleet/Unit.h>
#include <Mission/UnitMission.h>
#include <Nodes/Nodes.h>

#include <ApxMisc/JsonHelpers.h>

TelemetryRecorder::TelemetryRecorder(Unit *unit, Fact *parent)
    : Fact(parent, "recorder", tr("Recorder"), tr("Telemetry recording"))
    , _unit(unit)
{
    setIcon("record-rec");

    f_enable = new Fact(parent,
                        "enable",
                        tr("Record telemetry"),
                        tr("Enable telemetry recording"),
                        Bool);
    connect(f_enable, &Fact::valueChanged, this, &TelemetryRecorder::recordingChanged);
    connect(f_enable, &Fact::valueChanged, this, &TelemetryRecorder::restartRecording);

    // prepare fields map
    for (auto f : _unit->f_mandala->valueFacts()) {
        // guess best field storage format
        auto uid = f->uid();
        auto dspec = TelemetryFileWriter::dspec_for_uid(uid);
        _fields_map[uid] = {f->mpath(), {f->title(), f->units()}, dspec};
    }

    // record doenlink/uplink
    connect(unit->f_mandala, &Mandala::recordTelemetry, this, &TelemetryRecorder::recordTelemetry);
    connect(unit->f_mandala, &Mandala::recordData, this, &TelemetryRecorder::recordData);

    // record serial port data
    connect(unit->protocol()->data(),
            &PData::serialData,
            this,
            [this](quint8 portID, QByteArray data) { recordSerialData(portID, data, false); });

    // record config on each upload or save
    connect(unit->f_storage, &UnitStorage::confSaved, this, &TelemetryRecorder::recordUnitConf);

    // record text messages
    connect(unit->f_nodes, &Nodes::fieldUploadReport, this, &TelemetryRecorder::recordConfigUpdate);

    connect(AppNotify::instance(),
            &AppNotify::notification,
            this,
            &TelemetryRecorder::recordNotification);

    connect(unit, &Unit::messageReported, this, &TelemetryRecorder::recordMsg);

    // record mission on each upload or download
    connect(unit->f_mission, &UnitMission::missionDownloaded, this, [this]() {
        recordMission(false);
    });
    connect(unit->f_mission, &UnitMission::missionUploaded, this, [this]() { recordMission(true); });

    // display
    connect(this, &TelemetryRecorder::timeChanged, this, &TelemetryRecorder::updateStatus);

    timeUpdateTimer.setSingleShot(true);
    timeUpdateTimer.setInterval(500);
    connect(&timeUpdateTimer, &QTimer::timeout, this, &TelemetryRecorder::timeUpdate);

    // auto recorder
    flightState_s = Unit::FS_UNKNOWN;
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

    QString unitName = _unit->title();
    if (unitName.isEmpty())
        unitName = _unit->confTitle();

    const auto info = prepareFileInfo();
    const auto time_utc = timestamp.toMSecsSinceEpoch();

    // create new file
    const auto basename = db::storage::Session::telemetryFileBasename(timestamp, unitName);
    const auto filePath = db::storage::Session::telemetryFilePathUnique(basename);
    if (filePath.isEmpty())
        return;

    auto fileDir = QFileInfo(filePath).absoluteDir();
    if (!fileDir.exists())
        fileDir.mkpath(".");

    _stream_file.setFileName(filePath);
    if (!_stream_file.open(QIODevice::WriteOnly)) {
        apxMsgW() << tr("Failed to open file").append(':') << filePath;
        return;
    }
    _stream.init(&_stream_file, basename, time_utc, info);

    // record initial mandala state
    for (auto f : _unit->f_mandala->valueFacts()) {
        if (!(f->everReceived() || f->everSent()))
            continue;
        const auto &field = _fields_map[f->uid()];
        _stream.write_value(&field, f->value());
    }

    // record initial meta data
    if (_unit->f_nodes->valid()) {
        auto hash = _configHash;
        _configHash.clear();
        recordUnitConf(hash, _unit->confTitle());
    }

    if (!_unit->f_mission->empty())
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

    info["unit"] = _unit->get_info();
    info["conf"] = _unit->confTitle();

    QJsonObject sw;
    sw["version"] = QCoreApplication::applicationVersion();
    sw["hash"] = App::git_hash();
    info["sw"] = sw;

    info["host"] = App::host();

    return info;
}

TelemetryFileWriter::Values TelemetryRecorder::prepareValues(const PBase::Values &values) const
{
    TelemetryFileWriter::Values vlist;
    for (const auto [uid, value] : values) {
        auto it = _fields_map.find(uid);
        if (it == _fields_map.end()) {
            qWarning() << "field not found" << uid << value << _fields_map.size();
            continue;
        }
        vlist[&it->second] = value;
    }
    return vlist;
}

quint64 TelemetryRecorder::getEventTimestamp()
{
    if (!_tsElapsed.isValid()) {
        _tsElapsed.start();
        qDebug() << _unit->title() << "0 PRE";
        return 0;
    }

    quint64 elapsed = _tsElapsed.elapsed();
    quint64 t = elapsed + _ts_t2;

    // qDebug() << _unit->title() << t << elapsed << _ts_t2;
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
        qDebug() << _unit->title() << "reset timestamp:" << t;
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

    // qDebug() << _unit->title() << t << values.size();

    _stream.write_values(t, prepareValues(values), false);
}
void TelemetryRecorder::recordData(PBase::Values values, bool uplink)
{
    // qDebug() << values << uplink;

    checkFileRecord();

    _stream.write_values(getEventTimestamp(), prepareValues(values), uplink);
}

// write data slots

void TelemetryRecorder::recordNotification(QString msg,
                                           QString subsystem,
                                           AppNotify::NotifyFlags flags,
                                           Fact *fact)
{
    if (msg.isEmpty())
        return;
    if (flags & AppNotify::FromUnit)
        return;

    auto uid = fact ? fact->path() : QString();
    auto src = "gcs" + (subsystem.isEmpty() ? "" : ("/" + subsystem));
    _stream.write_evt(getEventTimestamp(), &telemetry::EVT_MSG, {uid, src, msg}, false, 1);
}

void TelemetryRecorder::recordMsg(QString msg, QString subsystem, QString src_uid)
{
    if (msg.isEmpty())
        return;
    _stream.write_evt(getEventTimestamp(), &telemetry::EVT_MSG, {src_uid, subsystem, msg}, false, 1);
}

void TelemetryRecorder::recordConfigUpdate(NodeItem *node, QString name, QString value)
{
    name = QString("%1/%2").arg(node->title()).arg(name);
    _stream.write_evt(getEventTimestamp(), &telemetry::EVT_CONF, {node->uid(), name, value}, true);
}
void TelemetryRecorder::recordSerialData(quint16 portNo, QByteArray data, bool uplink)
{
    QByteArray d;
    d.append((char) portNo, 2);
    d.append(data);
    _stream.write_raw(getEventTimestamp(), "vcp", d, uplink);
}

void TelemetryRecorder::recordMission(bool uplink)
{
    if (_unit->f_mission->empty())
        return;
    auto mission = _unit->f_mission->toJson().toObject();
    auto title = mission["title"].toString();
    auto hash = mission["hash"].toString();

    _stream.write_jso(getEventTimestamp(), "mission", mission, uplink);
}
void TelemetryRecorder::recordUnitConf(QString hash, QString title)
{
    if (hash.isEmpty())
        return;
    if (_configHash == hash)
        return;
    _configHash = hash;

    _stream.write_jso(getEventTimestamp(), "nodes", _unit->toJson().toObject(), false);
}

bool TelemetryRecorder::checkAutoRecord(void)
{
    if (_unit->streamType() != PUnit::TELEMETRY)
        return recording();

    Unit::FlightState fs = _unit->flightState();
    if (flightState_s != fs) {
        flightState_s = fs; //only once

        // stop recording when landed
        if (fs == Unit::FS_LANDED && recording() && !recStopTimer.isActive())
            recStopTimer.start(2000);

        // start or restart when starts flying
        if (fs == Unit::FS_TAKEOFF) {
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
