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
#include "Telemetry.h"
#include "LookupTelemetry.h"
#include "TelemetryPlayer.h"
#include "TelemetryReader.h"
#include "TelemetryRecorder.h"
//#include "TelemetryShare.h"

#include <App/App.h>
#include <Mission/MissionStorage.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
Telemetry::Telemetry(Vehicle *parent)
    : Fact(parent,
           "telemetry",
           tr("Telemetry"),
           tr("Telemetry data recorder"),
           Group,
           "inbox-arrow-down")
    , vehicle(parent)
    , f_recorder(nullptr)
    , f_lookup(nullptr)
    , f_reader(nullptr)
    , f_share(nullptr)
{
    if (vehicle->isReplay()) {
        setOpt("pos", QPointF(1, 1));

        f_lookup = new LookupTelemetry(this);
        f_lookup->f_latest->createAction(this);
        f_lookup->f_prev->createAction(this);
        f_lookup->f_next->createAction(this);
        f_reader = new TelemetryReader(f_lookup, this);
        connect(f_reader, &Fact::valueChanged, this, &Telemetry::updateStatus);
        connect(f_reader, &Fact::progressChanged, this, &Telemetry::updateProgress);
        connect(f_reader,
                &TelemetryReader::recordFactTriggered,
                this,
                &Telemetry::recordFactTriggered);

        connect(f_reader,
                &TelemetryReader::dataAvailable,
                this,
                &Telemetry::recordLoaded,
                Qt::QueuedConnection);

        f_player = new TelemetryPlayer(this, this);
        connect(f_player, &Fact::valueChanged, this, &Telemetry::updateStatus);
        bindProperty(f_player, "active", true);

        //FIXME: share
        /*f_share = new TelemetryShare(this, this);
        connect(f_share, &TelemetryShare::importJobDone, this, [this](quint64 id) {
            f_lookup->jumpToRecord(id);
        });
        connect(f_share, &Fact::progressChanged, this, &Telemetry::updateProgress);*/

        connect(App::instance(), &App::loadingFinished, this, [this]() {
            connect(vehicle,
                    &Vehicle::selected,
                    f_reader,
                    &TelemetryReader::loadCurrent,
                    Qt::QueuedConnection);
        });

    } else {
        f_recorder = new TelemetryRecorder(vehicle, this);
        connect(f_recorder, &TelemetryRecorder::recordingChanged, this, [this]() {
            setActive(f_recorder->recording());
        });
        connect(f_recorder, &Fact::valueChanged, this, &Telemetry::updateStatus);
    }
    descr_s = descr();
    updateStatus();
}
//=============================================================================
void Telemetry::updateStatus()
{
    if (f_recorder)
        setValue(f_recorder->value());
    if (f_reader)
        setValue(
            QString("%1/%2").arg(f_player->value().toString()).arg(f_reader->value().toString()));
}
void Telemetry::updateProgress()
{
    if (!f_reader)
        return;
    int v = f_reader->progress();
    // if (v < 0 && f_share)
    //     v = f_share->progress();
    setProgress(v);
    updateDescr();
}
void Telemetry::updateDescr()
{
    if (!f_reader)
        return;
    if (f_reader->progress() >= 0) {
        setDescr(tr("Reading").append("..."));
    } /*else if (f_share && f_share->progress() >= 0) {
        setDescr(f_share->descr());
    } */
    else
        setDescr(descr_s);
}
//=============================================================================
void Telemetry::recordFactTriggered(Fact *f)
{
    const QString &s = f->name();
    const QString &uid = f->descr();
    if (s.startsWith("nodes")) {
        //TODO: vehicle->protocol()->storage->loadConfiguration(uid);
    } else if (s.startsWith("mission")) {
        vehicle->f_mission->storage->loadMission(uid);
    } else {
        if (f_player)
            f_player->f_time->setValue(f->property("time").toULongLong() - 1);
    }
}
//=============================================================================
void Telemetry::recordLoaded()
{
    //vehicle->f_select->trigger();

    vehicle->setGeoPath(f_reader->geoPath);

    Fact *f_events = f_reader->child("events");
    if (f_events) {
        Fact *f = f_events->child("mission");
        if (f && f->size() > 0) {
            f = f->child(0);
            if (f)
                f->trigger();
        }
        f = f_events->child("nodes");
        if (f && f->size() > 0) {
            f = f->child(f->size() - 1);
            if (f)
                f->trigger();
        }
    }
}
//=============================================================================
