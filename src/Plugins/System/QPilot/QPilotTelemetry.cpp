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
#include <limits>

namespace {

QString cleanId(QString id)
{
    id = id.trimmed();

    while (id.contains(QStringLiteral(".."))) {
        id.replace(QStringLiteral(".."), QStringLiteral("."));
    }

    if (id.startsWith(QLatin1Char('.'))) {
        id.remove(0, 1);
    }

    if (id.endsWith(QLatin1Char('.'))) {
        id.chop(1);
    }

    return id;
}

QString factSearchText(const QString &id, Fact *fact)
{
    if (!fact) {
        return id.toLower();
    }

    return QStringLiteral("%1 %2 %3 %4 %5")
        .arg(id, fact->name(), fact->title(), fact->descr(), fact->units())
        .toLower();
}

bool looksLikeBatteryText(const QString &text)
{
    return text.contains(QStringLiteral("bat"))
        || text.contains(QStringLiteral("battery"))
        || text.contains(QStringLiteral("volt"))
        || text.contains(QStringLiteral("voltage"))
        || text.contains(QStringLiteral("cell"))
        || text.contains(QStringLiteral("soc"))
        || text.contains(QStringLiteral("charge"))
        || text.contains(QStringLiteral("remain"))
        || text.contains(QStringLiteral("power"))
        || text.contains(QStringLiteral("energy"))
        || text.contains(QStringLiteral("mah"))
        || text.contains(QStringLiteral("mwh"));
}

bool looksLikeSocText(const QString &text)
{
    return text.contains(QStringLiteral("soc"))
        || text.contains(QStringLiteral("charge"))
        || text.contains(QStringLiteral("remain"))
        || text.contains(QStringLiteral("percent"))
        || text.contains(QLatin1Char('%'));
}

bool looksLikeVoltageText(const QString &text, Fact *fact)
{
    const QString units = fact ? fact->units().trimmed().toLower() : QString();

    return text.contains(QStringLiteral("volt"))
        || text.contains(QStringLiteral("voltage"))
        || text.contains(QStringLiteral("cell"))
        || text.contains(QStringLiteral("bat"))
        || text.contains(QStringLiteral("battery"))
        || units == QStringLiteral("v")
        || units == QStringLiteral("volt")
        || units == QStringLiteral("volts");
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
        << _facts.size();
}

void QPilotTelemetry::stop()
{
    _timer.stop();
}

void QPilotTelemetry::insertFact(const QString &id, Fact *fact)
{
    const QString key = cleanId(id);

    if (!fact || key.isEmpty()) {
        return;
    }

    _facts.insert(key, fact);
}

void QPilotTelemetry::insertAlias(const QString &id, Fact *fact)
{
    insertFact(id, fact);
}

void QPilotTelemetry::collectFactTree(Fact *fact, const QString &prefix, int depth)
{
    if (!fact || depth > 64) {
        return;
    }

    const QString name = fact->name().trimmed();
    const QString id = name.isEmpty()
        ? cleanId(prefix)
        : cleanId(prefix.isEmpty() ? name : prefix + QLatin1Char('.') + name);

    const auto children = fact->facts();

    if (children.isEmpty()) {
        QString key = id;

        if (key.isEmpty()) {
            key = cleanId(fact->titlePath(QLatin1Char('.')));
        }

        insertFact(key, fact);
        return;
    }

    for (Fact *child : children) {
        collectFactTree(child, id, depth + 1);
    }
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

    collectFactTree(unit->f_mandala, QString());

    insertAlias(QStringLiteral("est.att.roll"), unit->f_mandala->fact(mandala::est::nav::att::roll::uid));
    insertAlias(QStringLiteral("est.att.pitch"), unit->f_mandala->fact(mandala::est::nav::att::pitch::uid));
    insertAlias(QStringLiteral("est.att.yaw"), unit->f_mandala->fact(mandala::est::nav::att::yaw::uid));

    insertAlias(QStringLiteral("est.air.airspeed"), unit->f_mandala->fact(mandala::est::nav::air::airspeed::uid));

    insertAlias(QStringLiteral("est.pos.lat"), unit->f_mandala->fact(mandala::est::nav::pos::lat::uid));
    insertAlias(QStringLiteral("est.pos.lon"), unit->f_mandala->fact(mandala::est::nav::pos::lon::uid));
    insertAlias(QStringLiteral("est.pos.bearing"), unit->f_mandala->fact(mandala::est::nav::pos::bearing::uid));
    insertAlias(QStringLiteral("est.pos.speed"), unit->f_mandala->fact(mandala::est::nav::pos::speed::uid));
    insertAlias(QStringLiteral("est.pos.altitude"), unit->f_mandala->fact(mandala::est::nav::pos::hmsl::uid));
    insertAlias(QStringLiteral("est.pos.vspeed"), unit->f_mandala->fact(mandala::est::nav::pos::vspeed::uid));

    qInfo().noquote() << "[QPilot] mandala facts bound:" << _facts.size() << "facts";
}

QStringList QPilotTelemetry::trackedIds() const
{
    QStringList ids = _facts.keys();
    ids.removeDuplicates();
    ids.sort();
    return ids;
}

bool QPilotTelemetry::isTracked(const QString &id) const
{
    return _facts.contains(id);
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

    for (auto it = _facts.constBegin(); it != _facts.constEnd(); ++it) {
        const QString id = it.key();
        const FactValue value = readFact(it.value());

        _current.insert(id, value);

        if (value.ok && value.numeric) {
            values.insert(id, value.number);
        }
    }

    if (!values.isEmpty()) {
        _stats.ingestBatch(values, QDateTime::currentMSecsSinceEpoch(), false);
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

    Fact *fact = _facts.value(id, nullptr);

    if (fact) {
        out["name"] = fact->name();
        out["title"] = fact->title();
        out["descr"] = fact->descr();
        out["units"] = fact->units();
    }

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

double QPilotTelemetry::estimateLipoSocPercent(double cellVoltage)
{
    struct Point
    {
        double voltage;
        double percent;
    };

    static const QList<Point> table{
        {4.20, 100.0},
        {4.15, 95.0},
        {4.10, 90.0},
        {4.05, 85.0},
        {4.00, 80.0},
        {3.95, 72.0},
        {3.90, 65.0},
        {3.85, 58.0},
        {3.80, 50.0},
        {3.75, 40.0},
        {3.70, 30.0},
        {3.65, 22.0},
        {3.60, 15.0},
        {3.55, 8.0},
        {3.50, 5.0},
        {3.40, 2.0},
        {3.30, 0.0},
    };

    if (cellVoltage >= table.first().voltage) {
        return table.first().percent;
    }

    if (cellVoltage <= table.last().voltage) {
        return table.last().percent;
    }

    for (int i = 1; i < table.size(); ++i) {
        const Point high = table.at(i - 1);
        const Point low = table.at(i);

        if (cellVoltage <= high.voltage && cellVoltage >= low.voltage) {
            const double k = (cellVoltage - low.voltage) / (high.voltage - low.voltage);
            return low.percent + k * (high.percent - low.percent);
        }
    }

    return 0.0;
}

QStringList QPilotTelemetry::batteryCandidateIds() const
{
    QStringList out;

    for (auto it = _facts.constBegin(); it != _facts.constEnd(); ++it) {
        const QString text = factSearchText(it.key(), it.value());

        if (looksLikeBatteryText(text)) {
            out.append(it.key());
        }
    }

    out.removeDuplicates();
    out.sort();

    return out;
}

QJsonObject QPilotTelemetry::batteryJson(int cells) const
{
    if (cells <= 0) {
        cells = 6;
    }

    const QStringList candidates = batteryCandidateIds();

    QJsonArray candidateArray;
    for (const QString &id : candidates) {
        candidateArray.append(id);
    }

    QJsonObject out;
    out["ok"] = false;
    out["cells"] = cells;
    out["chemistry_assumed"] = "LiPo";
    out["method"] = "prefer_soc_fact_else_estimate_from_voltage_per_cell";
    out["candidate_ids"] = candidateArray;

    QString socId;
    double socPercent = std::numeric_limits<double>::quiet_NaN();

    QString packVoltageId;
    double packVoltage = std::numeric_limits<double>::quiet_NaN();

    QString cellVoltageId;
    double cellVoltage = std::numeric_limits<double>::quiet_NaN();

    for (const QString &id : candidates) {
        const FactValue value = _current.value(id);

        if (!value.ok || !value.numeric) {
            continue;
        }

        Fact *fact = _facts.value(id, nullptr);
        const QString text = factSearchText(id, fact);
        const double v = value.number;

        const bool batteryScoped =
            id.contains(QStringLiteral(".sns.bat."), Qt::CaseInsensitive)
            || id.contains(QStringLiteral(".battery."), Qt::CaseInsensitive)
            || text.contains(QStringLiteral("battery"));

        const bool socCandidate =
            batteryScoped
            && looksLikeSocText(text)
            && v > 0.0
            && v <= 100.0;

        if (socId.isEmpty() && socCandidate) {
            socId = id;
            socPercent = v;
            continue;
        }

        if (!looksLikeVoltageText(text, fact)) {
            continue;
        }

        if (cellVoltageId.isEmpty() && v >= 3.0 && v <= 4.35) {
            cellVoltageId = id;
            cellVoltage = v;
            continue;
        }

        if (packVoltageId.isEmpty()
            && v >= static_cast<double>(cells) * 3.0
            && v <= static_cast<double>(cells) * 4.35) {
            packVoltageId = id;
            packVoltage = v;
            continue;
        }
    }

    if (std::isfinite(socPercent)) {
        out["ok"] = true;
        out["source"] = "soc_fact";
        out["source_id"] = socId;
        out["charge_percent"] = socPercent;
        return out;
    }

    if (!std::isfinite(cellVoltage) && std::isfinite(packVoltage)) {
        cellVoltage = packVoltage / static_cast<double>(cells);
    }

    if (std::isfinite(cellVoltage)) {
        const double estimated = estimateLipoSocPercent(cellVoltage);

        out["ok"] = true;
        out["source"] = std::isfinite(packVoltage) ? "pack_voltage_estimate" : "cell_voltage_estimate";
        out["source_id"] = std::isfinite(packVoltage) ? packVoltageId : cellVoltageId;
        out["pack_voltage"] = std::isfinite(packVoltage)
            ? QJsonValue(packVoltage)
            : QJsonValue(cellVoltage * static_cast<double>(cells));
        out["cell_voltage"] = cellVoltage;
        out["charge_percent_estimated"] = estimated;
        out["note"] = "Voltage-based LiPo charge is approximate and depends on load and battery curve.";
        return out;
    }

    out["error"] = "no_battery_soc_or_voltage_fact_found";
    return out;
}

QJsonObject QPilotTelemetry::toolJson(const QString &idsCsv, int batteryCells) const
{
    QStringList ids;

    for (const QString &part : idsCsv.split(',', Qt::SkipEmptyParts)) {
        const QString id = cleanId(part);

        if (!id.isEmpty()) {
            ids.append(id);
        }
    }

    ids.removeDuplicates();

    const bool explicitIds = !ids.isEmpty();

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
    meta["total_tracked_facts"] = _facts.size();
    meta["correlations"] = "disabled_for_all_telemetry_collection";

    QJsonObject out;
    out["ok"] = true;
    out["server"] = "qpilot";
    out["mode"] = "mcp_only_all_mandala_leaf_facts";
    out["requested_ids"] = requested;
    out["explicit_ids"] = explicitIds;
    out["tracked_ids"] = QJsonArray::fromStringList(trackedIds());
    out["sample_period_ms"] = _timer.interval();
    out["facts"] = facts;
    out["battery"] = batteryJson(batteryCells);
    out["correlations"] = QJsonObject{
        {"total", QJsonObject{}},
        {"window", QJsonObject{}}
    };
    out["time_utc"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    out["meta"] = meta;

    return out;
}
