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
#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>

#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>

TelemetryPlayer::TelemetryPlayer(TelemetryReader *reader, Vehicle *vehicle, Fact *parent)
    : Fact(parent, "player", tr("Player"), tr("Telemetry data player"), Group)
    , reader(reader)
    , vehicle(vehicle)
    , setTime0(0)
    , blockTimeChange(false)
{
    setIcon("play-circle-outline");

    connect(reader, &TelemetryReader::rec_started, this, &TelemetryPlayer::rec_started);
    connect(reader, &TelemetryReader::rec_finished, this, &TelemetryPlayer::rec_finished);

    connect(&_file, &TelemetryFileReader::values, this, &TelemetryPlayer::rec_values);
    connect(&_file, &TelemetryFileReader::evt, this, &TelemetryPlayer::rec_evt);
    connect(&_file, &TelemetryFileReader::msg, this, &TelemetryPlayer::rec_msg);

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

    connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, [this](Vehicle *v) {
        if (v != this->vehicle)
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
    vehicle->f_select->trigger();
    setActive(true);

    vehicle->f_mandala->restoreDefaults();

    playTime0 = _time;
    playTime.start();

    tNext = _time;
    timer.start(0);
    _file.open();
    _values_init = false;

    // load nearest meta
    loadLatestMeta(reader->child("mission"), _time);
    loadLatestMeta(reader->child("nodes"), _time);
}

void TelemetryPlayer::loadLatestMeta(Fact *group, double time)
{
    if (!group)
        return;

    Fact *f = nullptr;
    for (auto i : group->facts()) {
        double t = i->property("time").toULongLong() / 1000.0;
        if (t > _time)
            break;
        f = i;
    }
    if (!f)
        f = group->child(0);
    if (f)
        f->trigger();
}

void TelemetryPlayer::stop()
{
    setActive(false);
    emit discardRequests();
    timer.stop();
    _file.close();
    _values.clear();
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
    _file.close();
}

void TelemetryPlayer::rec_finished()
{
    const auto &info = reader->info();
    auto path = info["info"]["path"].toString();
    if (path.isEmpty())
        return;
    _file.setFileName(path);

    // fill fields map
    _fieldsMap.clear();
    int idx = 0;
    for (const auto &i : reader->fields()) {
        _fieldsMap.insert(idx, vehicle->f_mandala->fact(i.name, true));
        idx++;
    }
}

void TelemetryPlayer::rec_values(quint64 timestamp_ms, TelemetryReader::Values data, bool uplink)
{
    if (!active())
        return;

    for (auto [idx, value] : data.asKeyValueRange()) {
        _values.insert(idx, value);

        if (uplink && _values_init) {
            auto f = _fieldsMap.value(idx);
            if (!f)
                continue;
            const QString &s = f->mpath();
            if (s.startsWith("cmd.rc."))
                continue;
            if (s.startsWith("cmd.gimbal."))
                continue;
            f->setValue(value);
            vehicle->message(QString("%1: %2 = %3").arg(">").arg(f->title()).arg(f->text()),
                             AppNotify::Important);
        }
    }
}
void TelemetryPlayer::rec_evt(
    quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink)
{
    if (!active())
        return;

    if (name == "conf") {
        loadConfValue(uid, value);
        if (_values_init) {
            QString fn = value.left(value.indexOf('='));
            if (value.size() > (fn.size() + 32) || value.contains('\n')) {
                value = fn + "=<data>";
            }
        }
    } else if (name == "mission") {
        vehicle->f_mission->storage->loadMission(uid);
    } else if (name == "nodes") {
        vehicle->storage()->loadVehicleConfig(uid);
    }

    if (!_values_init)
        return;

    AppNotify::NotifyFlags flags = AppNotify::Important;
    if (!uplink)
        flags |= AppNotify::FromVehicle;
    QString s = QString("%1: %2").arg(uplink ? ">" : "<").arg(name);
    if (!value.isEmpty())
        s.append(QString(" (%1)").arg(value));
    vehicle->message(s, flags);
}
void TelemetryPlayer::rec_msg(quint64 timestamp_ms, QString text, QString subsystem)
{
    if (!active() || !_values_init)
        return;
    vehicle->message(QString("<: %1").arg(text),
                     AppNotify::FromVehicle | AppNotify::Important,
                     subsystem);
    App::sound(text);
}

void TelemetryPlayer::loadConfValue(const QString &sn, QString s)
{
    NodeItem *node = vehicle->f_nodes->node(sn);
    if (!node) {
        qWarning() << "missing node" << sn;
        return;
    }
    int del = s.indexOf('=');
    if (del < 0)
        return;
    QString spath = s.left(del).trimmed();
    QString sv = s.mid(del + 1);
    if (spath.startsWith(node->title()))
        spath.remove(0, node->title().size() + 1);
    if (spath.isEmpty())
        return;
    node->loadConfigValue(spath, sv);
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
        ok = _file.parse_next();
        if (!ok || _file.atEnd())
            break;
        auto t1 = _file.current_time();
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

    if (updated || !ok) {
        for (auto [idx, value] : _values.asKeyValueRange()) {
            auto f = _fieldsMap.value(idx);
            if (f)
                f->setValue(value);
        }
        _values.clear();
        vehicle->f_mandala->telemetryDecoded();
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
