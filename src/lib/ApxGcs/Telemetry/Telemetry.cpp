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
#include "Telemetry.h"
#include "LookupTelemetry.h"
#include "TelemetryPlayer.h"
#include "TelemetryReader.h"
#include "TelemetryRecorder.h"
#include "TelemetryShare.h"

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
        connect(f_reader, &Fact::statusChanged, this, &Telemetry::updateStatus);
        connect(f_reader, &Fact::progressChanged, this, &Telemetry::updateProgress);
        connect(f_reader,
                &TelemetryReader::recordFactTriggered,
                this,
                &Telemetry::recordFactTriggered);

        connect(f_reader, &TelemetryReader::dataAvailable, this, &Telemetry::recordLoaded);

        f_player = new TelemetryPlayer(this, this);
        connect(f_player, &Fact::statusChanged, this, &Telemetry::updateStatus);
        connect(f_player, &Fact::activeChanged, this, [=]() { setActive(f_player->active()); });

        f_share = new TelemetryShare(this, this);
        connect(f_share, &TelemetryShare::importJobDone, this, [this](quint64 id) {
            f_lookup->jumpToRecord(id);
        });
        connect(f_share, &Fact::progressChanged, this, &Telemetry::updateProgress);

        connect(vehicle, &Vehicle::selected, f_reader, &TelemetryReader::loadCurrent);

    } else {
        f_recorder = new TelemetryRecorder(vehicle, this);
        connect(f_recorder, &Fact::valueChanged, this, [=]() {
            setActive(f_recorder->value().toBool());
        });
        connect(f_recorder, &Fact::statusChanged, this, &Telemetry::updateStatus);
    }
    descr_s = descr();
    updateStatus();
}
//=============================================================================
void Telemetry::updateStatus()
{
    if (f_recorder)
        setStatus(f_recorder->status());
    if (f_reader)
        setStatus(QString("%1/%2").arg(f_player->status()).arg(f_reader->status()));
}
void Telemetry::updateProgress()
{
    if (!f_reader)
        return;
    int v = f_reader->progress();
    if (v < 0 && f_share)
        v = f_share->progress();
    setProgress(v);
    updateDescr();
}
void Telemetry::updateDescr()
{
    if (!f_reader)
        return;
    if (f_reader->progress() >= 0) {
        setDescr(tr("Reading").append("..."));
    } else if (f_share && f_share->progress() >= 0) {
        setDescr(f_share->descr());
    } else
        setDescr(descr_s);
}
//=============================================================================
void Telemetry::recordFactTriggered(Fact *f)
{
    const QString &s = f->name();
    const QString &uid = f->descr();
    if (s.startsWith("nodes")) {
        vehicle->f_nodes->storage->loadConfiguration(uid);
    } else if (s.startsWith("mission")) {
        vehicle->f_mission->storage->loadMission(uid);
    } else {
        if (f_player)
            f_player->f_time->setValue(f->userData.toULongLong() - 1);
    }
}
//=============================================================================
void Telemetry::recordLoaded()
{
    vehicle->f_select->trigger();

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
