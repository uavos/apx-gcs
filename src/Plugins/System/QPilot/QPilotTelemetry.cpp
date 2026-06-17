/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2026, QPilot contributors
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
#include "QPilotTelemetry.h"

#include <Fact/Fact.h>
#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Mandala/Mandala.h>

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>

#include <cmath>

namespace {

using MandalaUid = decltype(mandala::est::nav::att::roll::uid);

struct TrackedFactDef
{
    QString id;
    MandalaUid uid;
};

const QList<TrackedFactDef> &trackedFactDefs()
{
    static const QList<TrackedFactDef> facts{
        {QStringLiteral("est.att.roll"), mandala::est::nav::att::roll::uid},
        {QStringLiteral("est.att.pitch"), mandala::est::nav::att::pitch::uid},
        {QStringLiteral("est.att.yaw"), mandala::est::nav::att::yaw::uid},
        {QStringLiteral("est.air.airspeed"), mandala::est::nav::air::airspeed::uid},
        {QStringLiteral("est.pos.speed"), mandala::est::nav::pos::speed::uid},
        {QStringLiteral("est.pos.altitude"), mandala::est::nav::pos::hmsl::uid},
        {QStringLiteral("est.pos.vspeed"), mandala::est::nav::pos::vspeed::uid},
    };

    return facts;
}

} // namespace

QPilotTelemetry::QPilotTelemetry(QObject *parent)
    : QObject(parent)
{
    _timer.setInterval(200);
    _timer.setTimerType(Qt::CoarseTimer);

    connect(&_timer, &QTimer::timeout, this, &QPilotTelemetry::sampleFacts);

    connect(Fleet::instance(), &Fleet::unitSelected, this, [this](Unit *unit) {
        bindUnit(unit);
    });

    bindUnit(Fleet::instance()->current());
}

void QPilotTelemetry::start()
{
    sampleFacts();

    if (!_timer.isActive()) {
        _timer.start();
    }

    qInfo().noquote()
        << "[QPilot] telemetry timer started, tracked facts:"
        << trackedIds().join(", ");
}

void QPilotTelemetry::stop()
{
    _timer.stop();
}

void QPilotTelemetry::bindUnit(Unit *unit)
{
    _facts.clear();
    _current.clear();
    _stats = QPilotStats();

    if (!unit || !unit->f_mandala) {
        qWarning().noquote() << "[QPilot] no current unit mandala";
        return;
    }

    for (const TrackedFactDef &def : trackedFactDefs()) {
        _facts.insert(def.id, unit->f_mandala->fact(def.uid));
    }

    qInfo().noquote() << "[QPilot] mandala facts bound:" << trackedIds().join(", ");
}

QStringList QPilotTelemetry::trackedIds() const
{
    QStringList ids;

    for (const TrackedFactDef &def : trackedFactDefs()) {
        ids.append(def.id);
    }

    return ids;
}

bool QPilotTelemetry::isTracked(const QString &id) const
{
    for (const TrackedFactDef &def : trackedFactDefs()) {
        if (def.id == id) {
            return true;
        }
    }

    return false;
}

QPilotTelemetry::FactValue QPilotTelemetry::readFact(Fact *fact) const
{
    FactValue out;
    out.timestampMs = QDateTime::currentMSecsSinceEpoch();

    if (!fact) {
        out.error = QStringLiteral("fact_not_available");
        return out;
    }

    out.ok = true;
    out.raw = fact->valueText();

    bool ok = false;
    const double number = fact->value().toDouble(&ok);

    if (ok && std::isfinite(number)) {
        out.numeric = true;
        out.number = number;
    }

    return out;
}

void QPilotTelemetry::sampleFacts()
{
    QMap<QString, double> values;

    for (const TrackedFactDef &def : trackedFactDefs()) {
        const FactValue value = readFact(_facts.value(def.id, nullptr));

        _current.insert(def.id, value);

        if (value.ok && value.numeric) {
            values.insert(def.id, value.number);
        }
    }

    if (!values.isEmpty()) {
        _stats.ingestBatch(values, QDateTime::currentMSecsSinceEpoch());
    }
}

QJsonObject QPilotTelemetry::factPayloadJson(const QString &id) const
{
    if (!isTracked(id)) {
        return QJsonObject{
            {"fact", id},
            {"ok", false},
            {"error", "fact_not_tracked"}
        };
    }

    if (!_current.contains(id)) {
        return QJsonObject{
            {"fact", id},
            {"ok", false},
            {"error", "fact_not_sampled_yet"}
        };
    }

    const FactValue value = _current.value(id);

    QJsonObject out;
    out["fact"] = id;
    out["ok"] = value.ok;
    out["updated_utc"] = value.timestampMs > 0
        ? QDateTime::fromMSecsSinceEpoch(value.timestampMs, Qt::UTC).toString(Qt::ISODateWithMs)
        : QString();

    if (!value.ok) {
        out["error"] = value.error;
        return out;
    }

    out["raw"] = value.raw;
    out["numeric"] = value.numeric;
    out["value"] = value.numeric ? QJsonValue(value.number) : QJsonValue(value.raw);

    if (value.numeric) {
        const QJsonObject stats = _stats.statsJson(id, value.number);

        for (auto it = stats.constBegin(); it != stats.constEnd(); ++it) {
            out[it.key()] = it.value();
        }
    }

    return out;
}

QJsonObject QPilotTelemetry::toolJson(const QString &idsCsv) const
{
    QStringList ids;

    for (const QString &part : idsCsv.split(',', Qt::SkipEmptyParts)) {
        const QString id = part.trimmed();

        if (!id.isEmpty()) {
            ids.append(id);
        }
    }

    ids.removeDuplicates();

    if (ids.isEmpty()) {
        ids = trackedIds();
    }

    QJsonArray requested;
    QJsonObject facts;

    for (const QString &id : ids) {
        requested.append(id);
        facts[id] = factPayloadJson(id);
    }

    QJsonObject meta;
    meta["read_only"] = true;
    meta["raw_history_output"] = false;
    meta["window_s"] = QPilotStats::windowSeconds();

    QJsonObject out;
    out["ok"] = true;
    out["server"] = "qpilot";
    out["mode"] = "mcp_only_direct_mandala_tracking";
    out["requested_ids"] = requested;
    out["tracked_ids"] = QJsonArray::fromStringList(trackedIds());
    out["sample_period_ms"] = _timer.interval();
    out["facts"] = facts;
    out["correlations"] = _stats.correlationsJson(ids);
    out["time_utc"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    out["meta"] = meta;

    return out;
}
