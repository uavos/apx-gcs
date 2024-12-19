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
#include "TelemetryPlayer.h"
#include "TelemetryReader.h"
#include "TelemetryRecorder.h"
#include "TelemetryRecords.h"
#include "TelemetryShare.h"

#include <App/App.h>
#include <Fleet/Fleet.h>
#include <Mission/UnitMission.h>
#include <Nodes/Nodes.h>

Telemetry::Telemetry(Unit *parent)
    : Fact(parent,
           "telemetry",
           tr("Telemetry"),
           tr("Telemetry data recorder"),
           Group,
           "inbox-arrow-down")
    , unit(parent)
    , f_recorder(nullptr)
    , f_records(nullptr)
    , f_reader(nullptr)
    , f_share(nullptr)
{
    if (unit->isReplay()) {
        setOpt("pos", QPointF(1, 1));

        f_records = new TelemetryRecords(this);
        f_reader = new TelemetryReader(this);

        connect(f_records,
                &TelemetryRecords::recordTriggered,
                f_reader,
                &TelemetryReader::loadRecord);
        connect(f_reader,
                &TelemetryReader::recordInfoUpdated,
                f_records,
                &TelemetryRecords::recordInfoUpdated);

        connect(f_reader, &Fact::valueChanged, this, &Telemetry::updateStatus);
        connect(f_reader, &Fact::progressChanged, this, &Telemetry::updateProgress);
        connect(f_reader,
                &TelemetryReader::statsFactTriggered,
                this,
                &Telemetry::statsFactTriggered);

        connect(f_reader,
                &TelemetryReader::rec_finished,
                this,
                &Telemetry::recordLoaded,
                Qt::QueuedConnection);
        connect(f_reader,
                &TelemetryReader::geoPathCollected,
                this,
                [this](const QGeoPath &path, quint64 totalDistance) {
                    unit->setGeoPath(path);
                    unit->setTotalDistance(totalDistance);
                });

        f_player = new TelemetryPlayer(f_reader, unit, this);
        connect(f_player, &Fact::valueChanged, this, &Telemetry::updateStatus);
        bindProperty(f_player, "active", true);

        f_share = new TelemetryShare(this, this);
        connect(f_share, &Fact::progressChanged, this, &Telemetry::updateProgress);

        f_records->f_prev->createAction(this);
        f_records->f_next->createAction(this);
        f_records->f_latest->createAction(this);

    } else {
        f_recorder = new TelemetryRecorder(unit, this);
        connect(f_recorder, &TelemetryRecorder::recordingChanged, this, [this]() {
            setActive(f_recorder->recording());
        });
        connect(f_recorder, &Fact::valueChanged, this, &Telemetry::updateStatus);
    }
    descr_s = descr();
    updateStatus();
}

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

void Telemetry::statsFactTriggered(Fact *f, QJsonObject jso)
{
    const QString &s = f->name();
    const QString &uid = f->descr();
    if (s.startsWith("nodes")) {
        qDebug() << "load telemetry nodes";
        unit->fromJson(jso);
    } else if (s.startsWith("mission")) {
        qDebug() << "load telemetry mission";
        unit->f_mission->fromJson(jso);
    } else {
        if (f_player)
            f_player->f_time->setValue(f->property("time").toULongLong() - 1);
    }
}

void Telemetry::recordLoaded()
{
    if (!Fleet::instance()->current()->isIdentified())
        unit->f_select->trigger();

    // reset mandala counters on file reload
    unit->f_mandala->resetCounters();

    // load latest mission
    Fact *f = f_reader->child("mission");
    if (f && f->size() > 0) {
        f = f->child(0);
        if (f)
            f->trigger();
    }

    // load latest config
    f = f_reader->child("nodes");
    if (f && f->size() > 0) {
        f = f->child(f->size() - 1);
        if (f)
            f->trigger();
    }
}
