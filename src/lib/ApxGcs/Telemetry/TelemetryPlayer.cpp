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
#include "TelemetryPlayer.h"
#include "Telemetry.h"
#include "TelemetryReader.h"
#include "TelemetryRecords.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <Mandala/MandalaAliases.h>
#include <Mission/UnitMission.h>
#include <Nodes/Nodes.h>

TelemetryPlayer::TelemetryPlayer(TelemetryReader *reader, Unit *unit, Fact *parent)
    : Fact(parent, "player", tr("Player"), tr("Telemetry data player"), Group)
    , reader(reader)
    , unit(unit)
    , setTime0(0)
    , blockTimeChange(false)
{
    setIcon("play-circle-outline");

    connect(reader, &TelemetryReader::rec_started, this, &TelemetryPlayer::rec_started);
    connect(reader, &TelemetryReader::rec_finished, this, &TelemetryPlayer::rec_finished);

    connect(&_stream, &TelemetryFileReader::field, this, &TelemetryPlayer::rec_field);
    connect(&_stream, &TelemetryFileReader::values, this, &TelemetryPlayer::rec_values);
    connect(&_stream, &TelemetryFileReader::evt, this, &TelemetryPlayer::rec_evt);
    connect(&_stream, &TelemetryFileReader::jso, this, &TelemetryPlayer::rec_jso);

    connect(this, &Fact::activeChanged, this, &TelemetryPlayer::updateActive);

    f_time = new Fact(this, "time", tr("Time"), tr("Current postition"), Int);
    f_time->setMin((quint64) 0);
    f_time->setUnits("ms");
    connect(f_time, &Fact::valueChanged, this, &TelemetryPlayer::updateTime);
    connect(f_time, &Fact::valueChanged, this, &TelemetryPlayer::updateStatus);

    f_speed = new Fact(this, "speed", tr("Speed"), tr("Playback speed"), Float);
    f_speed->setMin(0.1);
    f_speed->setMax(10);
    f_speed->setPrecision(1);
    f_speed->setValue(1);
    connect(f_speed, &Fact::valueChanged, this, &TelemetryPlayer::updateSpeed);

    //actions
    f_play = new Fact(this, "play", tr("Play"), tr("Play stream"), Action | Apply, "play");
    connect(f_play, &Fact::triggered, this, &TelemetryPlayer::play);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Pause playing"), Action | Stop, "stop");
    connect(f_stop, &Fact::triggered, this, &TelemetryPlayer::stop);

    f_rewind = new Fact(this, "rewind", tr("Rewind"), tr("Jump back"), Action, "rewind");
    connect(f_rewind, &Fact::triggered, this, &TelemetryPlayer::rewind);

    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &TelemetryPlayer::next);

    // connect(telemetry->f_records, &TelemetryRecords::recordIdChanged, this, &TelemetryPlayer::reset);
    connect(reader, &TelemetryReader::totalTimeChanged, this, [this]() {
        f_time->setMax(this->reader->totalTime());
    });

    connect(this, &Fact::activeChanged, this, &TelemetryPlayer::updateActions);

    connect(Fleet::instance(), &Fleet::unitSelected, this, [this](Unit *unit) {
        if (unit != this->unit)
            stop();
    });
    updateSpeed();
    updateStatus();
    updateActions();
}

void TelemetryPlayer::updateActions()
{
    bool enb = true;
    bool playing = active();
    f_play->setEnabled(enb && (!playing));
    f_stop->setEnabled(enb && (playing));
    f_rewind->setEnabled(enb);
}

void TelemetryPlayer::updateStatus()
{
    setValue(AppRoot::timeToString(f_time->value().toULongLong() / 1000, true));
}

void TelemetryPlayer::reset()
{
    stop();
    f_time->setValue(0);
    f_speed->setValue(1.0);
    updateActions();
}

void TelemetryPlayer::updateSpeed()
{
    _speed = f_speed->value().toDouble();
    if (active()) {
        playTime0 = _time;
        playTime.start();
        next();
    }
}
void TelemetryPlayer::updateActive()
{
    if ((!active())) {
        emit discardRequests();
    }
}
void TelemetryPlayer::updateTime()
{
    if (blockTimeChange)
        return;
    setTime0 = _time = f_time->value().toULongLong();
    if (active()) {
        stop();
        play();
    }
}

void TelemetryPlayer::play()
{
    if (active())
        return;
    if (!reader->totalTime())
        return;
    unit->f_select->trigger();
    setActive(true);

    apxMsg() << tr("Replay started");

    unit->f_mandala->restoreDefaults();
    _facts_by_index.clear();

    playTime0 = _time;
    playTime.start();

    tNext = _time;
    timer.start(0);
    _stream_file.open(QIODevice::ReadOnly);
    _stream.init(&_stream_file, QFileInfo(_stream_file.fileName()).completeBaseName());
    _values_init = false;

    // load nearest meta
    loadLatestMeta(reader->child("mission"), _time);
    loadLatestMeta(reader->child("nodes"), _time);
}

void TelemetryPlayer::loadLatestMeta(Fact *group, quint64 time)
{
    if (!group)
        return;

    Fact *f = group->child(0);
    for (auto i : group->facts()) {
        auto t = i->property("time").toULongLong();
        if (t > time)
            break;
        f = i;
    }
    if (!f)
        return;

    f->trigger();
}

void TelemetryPlayer::stop()
{
    setActive(false);
    emit discardRequests();
    timer.stop();
    _values.clear();

    _stream_file.close();
}
void TelemetryPlayer::rewind()
{
    bool bPlaying = active();
    stop();
    if (_time == setTime0)
        f_time->setValue(0);
    else {
        f_time->setValue(setTime0);
        if (bPlaying)
            play();
    }
}

void TelemetryPlayer::rec_started()
{
    _stream_file.close();
}

void TelemetryPlayer::rec_finished()
{
    const auto &filePath = reader->recordFilePath();
    _stream_file.setFileName(filePath);
}

void TelemetryPlayer::rec_field(TelemetryReader::Field field)
{
    // if (!active())
    //     return;

    // _facts_by_index.push_back(TelemetryReader::fieldFact(field));
}
void TelemetryPlayer::rec_values(quint64 timestamp_ms, TelemetryReader::Values data, bool uplink)
{
    if (!active())
        return;

    // just store values to update later
    for (const auto &[index, value] : data) {
        _values[index] = value;
    }

    // report uplink values
    if (!uplink || !_values_init)
        return;

    for (const auto &[index, value] : data) {
        auto f = updateMandalaFact(index, value);
        if (f) {
            auto s = f->mpath();
            if (s.startsWith("cmd.rc."))
                continue;
            if (s.startsWith("cmd.gimbal."))
                continue;
            unit->message(QString(">%1=%2").arg(f->title(), f->text()), AppNotify::Important);
            continue;
        }
        // field not found in current mandala
        auto s = reader->fields().value(index).name;
        if (s.isEmpty())
            continue;
        if (s.startsWith("rc_"))
            continue;
        unit->message(QString(">%1").arg(s), AppNotify::Important);
    }
}
void TelemetryPlayer::rec_evt(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink)
{
    if (!active())
        return;

    QString subsystem;

    if (name == telemetry::EVT_MSG.name) {
        name = data.value("txt").toString();
        subsystem = data.value("src").toString();

    } else if (name == telemetry::EVT_CONF.name) {
        auto uid = data.value("uid").toString();
        auto param = data.value("param").toString();
        auto value = data.value("value").toVariant().toString();
        loadConfValue(uid, param, value);

        if (value.size() > (param.size() + 32) || value.contains('\n'))
            value = "<data>";
        name = QString("%1=%2").arg(param).arg(value);
    }

    if (!_values_init || name.isEmpty())
        return;

    message(name, uplink, subsystem);
}
void TelemetryPlayer::rec_jso(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink)
{
    if (!active())
        return;

    if (name == "mission") {
        if (_values_init)
            unit->f_mission->fromJson(data);
    } else if (name == "nodes") {
        if (_values_init)
            unit->f_nodes->fromJson(data);
    }

    if (!_values_init || name.isEmpty())
        return;

    message(name, uplink, {});
}

MandalaFact *TelemetryPlayer::updateMandalaFact(size_t index, const QVariant &value)
{
    // find facts by name cache
    while (_facts_by_index.size() <= index) {
        _facts_by_index.push_back(reader->fieldFact(index));
    }

    auto f = _facts_by_index[index];
    if (f)
        f->setValue(value);
    return f;
}

void TelemetryPlayer::message(QString msg, bool uplink, QString subsystem)
{
    AppNotify::NotifyFlags flags = AppNotify::Important;
    if (!uplink)
        flags |= AppNotify::FromUnit;
    auto s = QString("%1: %2").arg(uplink ? ">" : "<").arg(msg);
    unit->message(s, flags, subsystem);
}

void TelemetryPlayer::loadConfValue(QString uid, QString param, QString value)
{
    auto node = unit->f_nodes->node(uid);
    if (!node) {
        qWarning() << "missing node" << uid;
        return;
    }
    if (param.startsWith(node->title()))
        param.remove(0, node->title().size() + 1);
    if (param.isEmpty())
        return;
    node->loadConfigValue(param, value);
}

void TelemetryPlayer::next()
{
    if (!active())
        return;

    quint64 t = playTime.elapsed();
    if (_speed > 0 && _speed != 1.0)
        t = t * _speed;
    t += playTime0;

    bool updated = false;
    bool ok = true;
    while (tNext <= t) {
        ok = _stream.parse_next();
        if (!ok || _stream.atEnd() || _stream.interrupted())
            break;
        auto t1 = _stream.current_time();
        if (t1 != tNext) {
            tNext = t1;
            updated = true;
        }
    }

    if (_time != t) {
        _time = t;
        blockTimeChange = true;
        f_time->setValue(_time);
        blockTimeChange = false;
    }

    // update data values
    if (updated || !ok) {
        for (const auto &[index, value] : _values) {
            updateMandalaFact(index, value);
        }
        _values.clear();
        unit->f_mandala->telemetryDecoded();
        _values_init = true;
    }

    if (!ok) {
        apxMsg() << tr("Replay finished");
        stop();
        return;
    }

    // schedule next call
    if (active())
        timer.start(tNext - t);
}
