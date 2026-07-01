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
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVariant>
#include <QXmlStreamReader>

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

QString ecamId(const QString &name)
{
    const QString clean = cleanId(name);

    return clean.isEmpty()
        ? QString()
        : QStringLiteral("ecam.") + clean;
}

QString derivedId(const QString &name)
{
    const QString clean = cleanId(name);

    return clean.isEmpty()
        ? QString()
        : QStringLiteral("haps.") + clean;
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

QPilotTelemetry::FactValue valueFromText(const QString &text, qint64 timestampMs)
{
    QPilotTelemetry::FactValue out;
    out.ok = true;
    out.raw = text.trimmed();
    out.timestampMs = timestampMs;

    bool ok = false;
    const double number = out.raw.toDouble(&ok);

    if (ok && std::isfinite(number)) {
        out.numeric = true;
        out.number = number;
    }

    return out;
}

bool numericValue(const QHash<QString, QPilotTelemetry::FactValue> &values,
                  const QString &id,
                  double *out)
{
    const auto it = values.constFind(id);

    if (it == values.constEnd() || !it.value().ok || !it.value().numeric) {
        return false;
    }

    if (out) {
        *out = it.value().number;
    }

    return true;
}

void insertDerived(QHash<QString, QPilotTelemetry::FactValue> *current,
                   QMap<QString, double> *statsValues,
                   const QString &id,
                   double value,
                   qint64 timestampMs)
{
    if (!current || !statsValues || id.isEmpty() || !std::isfinite(value)) {
        return;
    }

    QPilotTelemetry::FactValue out;
    out.ok = true;
    out.numeric = true;
    out.number = value;
    out.raw = QString::number(value, 'g', 12);
    out.timestampMs = timestampMs;

    current->insert(id, out);
    statsValues->insert(id, value);
}

} // namespace

QPilotTelemetry::QPilotTelemetry(QObject *parent)
    : QObject(parent)
    , _ecamUrl(QStringLiteral("http://172.29.100.57:9380/ecam"))
{
    _timer.setInterval(200);
    _timer.setTimerType(Qt::CoarseTimer);

    _ecamTimer.setInterval(1000);
    _ecamTimer.setTimerType(Qt::CoarseTimer);

    connect(&_timer, &QTimer::timeout, this, &QPilotTelemetry::sampleFacts);
    connect(&_ecamTimer, &QTimer::timeout, this, &QPilotTelemetry::fetchEcam);

    connect(Fleet::instance(), &Fleet::unitSelected, this, [this](Unit *unit) {
        bindUnit(unit);
    });

    bindUnit(Fleet::instance()->current());
}

void QPilotTelemetry::start()
{
    sampleFacts();
    fetchEcam();

    if (!_timer.isActive()) {
        _timer.start();
    }

    if (!_ecamTimer.isActive()) {
        _ecamTimer.start();
    }

    qInfo().noquote()
        << "[QPilot] telemetry timer started, tracked mandala facts:"
        << _facts.size()
        << "ecam url:"
        << _ecamUrl.toString();
}

void QPilotTelemetry::stop()
{
    _timer.stop();
    _ecamTimer.stop();

    if (_ecamReply) {
        _ecamReply->abort();
        _ecamReply->deleteLater();
        _ecamReply = nullptr;
    }
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

QStringList QPilotTelemetry::ecamIds() const
{
    QStringList ids = _ecamCurrent.keys();
    ids.removeDuplicates();
    ids.sort();
    return ids;
}

QStringList QPilotTelemetry::derivedIds() const
{
    QStringList ids = _derivedCurrent.keys();
    ids.removeDuplicates();
    ids.sort();
    return ids;
}

QStringList QPilotTelemetry::allIds() const
{
    QStringList ids;

    ids.append(trackedIds());
    ids.append(ecamIds());
    ids.append(derivedIds());

    ids.removeDuplicates();
    ids.sort();

    return ids;
}

bool QPilotTelemetry::isTracked(const QString &id) const
{
    return _facts.contains(id);
}

bool QPilotTelemetry::isEcamTracked(const QString &id) const
{
    return _ecamCurrent.contains(id);
}

bool QPilotTelemetry::isDerivedTracked(const QString &id) const
{
    return _derivedCurrent.contains(id);
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

void QPilotTelemetry::fetchEcam()
{
    if (_ecamReply) {
        return;
    }

    _ecamLastAttemptMs = QDateTime::currentMSecsSinceEpoch();

    QNetworkRequest request(_ecamUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QPilot/0.3"));

    _ecamReply = _network.get(request);

    connect(_ecamReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = _ecamReply;
        _ecamReply = nullptr;

        handleEcamReply(reply);
    });
}

void QPilotTelemetry::handleEcamReply(QNetworkReply *reply)
{
    if (!reply) {
        return;
    }

    const QByteArray data = reply->readAll();
    const QNetworkReply::NetworkError err = reply->error();
    const QString errText = reply->errorString();

    reply->deleteLater();

    if (err != QNetworkReply::NoError) {
        _ecamLastError = errText;

        qWarning().noquote()
            << "[QPilot] ECAM fetch failed:"
            << _ecamUrl.toString()
            << errText;

        return;
    }

    ingestEcamXml(data, QDateTime::currentMSecsSinceEpoch());
}

void QPilotTelemetry::ingestEcamXml(const QByteArray &xml, qint64 timestampMs)
{
    QXmlStreamReader reader(xml);
    QMap<QString, double> values;

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement()) {
            continue;
        }

        const QString tag = reader.name().toString().trimmed();

        if (tag.isEmpty() || tag == QStringLiteral("ecam")) {
            continue;
        }

        const QString id = ecamId(tag);

        if (id.isEmpty()) {
            continue;
        }

        const QString text = reader.readElementText(QXmlStreamReader::SkipChildElements).trimmed();
        const FactValue value = valueFromText(text, timestampMs);

        _ecamCurrent.insert(id, value);

        if (value.ok && value.numeric) {
            values.insert(id, value.number);
        }
    }

    if (reader.hasError()) {
        _ecamLastError = reader.errorString();

        qWarning().noquote()
            << "[QPilot] ECAM XML parse failed:"
            << _ecamLastError;

        return;
    }

    _ecamLastError.clear();
    _ecamLastOkMs = timestampMs;

    if (!values.isEmpty()) {
        _ecamStats.ingestBatch(values, timestampMs, false);
    }

    updateDerivedHaps(timestampMs);
}

void QPilotTelemetry::updateDerivedHaps(qint64 timestampMs)
{
    QMap<QString, double> values;

    double spCalcPwr = 0.0;
    double escCalcPwr = 0.0;

    if (numericValue(_ecamCurrent, QStringLiteral("ecam.sp_calc_pwr"), &spCalcPwr)
        && numericValue(_ecamCurrent, QStringLiteral("ecam.esc_calc_pwr"), &escCalcPwr)) {
        insertDerived(&_derivedCurrent,
                      &values,
                      derivedId(QStringLiteral("power_balance")),
                      spCalcPwr - escCalcPwr,
                      timestampMs);
    }

    double worstCellDelta = std::numeric_limits<double>::quiet_NaN();

    for (int i = 1; i <= 4; ++i) {
        double delta = 0.0;

        if (!numericValue(_ecamCurrent, QStringLiteral("ecam.mc%1_delta").arg(i), &delta)) {
            continue;
        }

        if (!std::isfinite(worstCellDelta) || delta > worstCellDelta) {
            worstCellDelta = delta;
        }
    }

    if (std::isfinite(worstCellDelta)) {
        insertDerived(&_derivedCurrent,
                      &values,
                      derivedId(QStringLiteral("battery_worst_cell_delta")),
                      worstCellDelta,
                      timestampMs);
    }

    double worstPitchError = std::numeric_limits<double>::quiet_NaN();

    for (int i = 1; i <= 2; ++i) {
        double pitch = 0.0;
        double cmdPitch = 0.0;

        if (!numericValue(_ecamCurrent, QStringLiteral("ecam.m_haps_pitch%1").arg(i), &pitch)
            || !numericValue(_ecamCurrent, QStringLiteral("ecam.m_haps_cmd_pitch%1").arg(i), &cmdPitch)) {
            continue;
        }

        const double error = std::abs(pitch - cmdPitch);

        if (!std::isfinite(worstPitchError) || error > worstPitchError) {
            worstPitchError = error;
        }
    }

    if (std::isfinite(worstPitchError)) {
        insertDerived(&_derivedCurrent,
                      &values,
                      derivedId(QStringLiteral("tracking_error_pitch_abs")),
                      worstPitchError,
                      timestampMs);
    }

    if (!values.isEmpty()) {
        _derivedStats.ingestBatch(values, timestampMs, false);
    }
}

QJsonObject QPilotTelemetry::factPayloadJson(const QString &id) const
{
    if (!isTracked(id)) {
        return QJsonObject{
            {"fact", id},
            {"source", "mandala"},
            {"ok", false},
            {"error", "fact_not_tracked"}
        };
    }

    if (!_current.contains(id)) {
        return QJsonObject{
            {"fact", id},
            {"source", "mandala"},
            {"ok", false},
            {"error", "fact_not_sampled_yet"}
        };
    }

    const FactValue value = _current.value(id);

    QJsonObject out;
    out["fact"] = id;
    out["source"] = "mandala";
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

QJsonObject QPilotTelemetry::ecamPayloadJson(const QString &id) const
{
    if (!isEcamTracked(id)) {
        return QJsonObject{
            {"fact", id},
            {"source", "ecam"},
            {"ok", false},
            {"error", "ecam_fact_not_tracked"}
        };
    }

    const FactValue value = _ecamCurrent.value(id);

    QJsonObject out;
    out["fact"] = id;
    out["source"] = "ecam";
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
        const QJsonObject stats = _ecamStats.statsJson(id, value.number);

        for (auto it = stats.constBegin(); it != stats.constEnd(); ++it) {
            out[it.key()] = it.value();
        }
    }

    return out;
}

QJsonObject QPilotTelemetry::derivedPayloadJson(const QString &id) const
{
    if (!isDerivedTracked(id)) {
        return QJsonObject{
            {"fact", id},
            {"source", "haps_derived"},
            {"ok", false},
            {"error", "derived_fact_not_tracked"}
        };
    }

    const FactValue value = _derivedCurrent.value(id);

    QJsonObject out;
    out["fact"] = id;
    out["source"] = "haps_derived";
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
        const QJsonObject stats = _derivedStats.statsJson(id, value.number);

        for (auto it = stats.constBegin(); it != stats.constEnd(); ++it) {
            out[it.key()] = it.value();
        }
    }

    return out;
}

QJsonObject QPilotTelemetry::telemetryPayloadJson(const QString &id) const
{
    if (isTracked(id)) {
        return factPayloadJson(id);
    }

    if (isEcamTracked(id)) {
        return ecamPayloadJson(id);
    }

    if (isDerivedTracked(id)) {
        return derivedPayloadJson(id);
    }

    return QJsonObject{
        {"fact", id},
        {"ok", false},
        {"error", "telemetry_id_not_tracked"}
    };
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

    for (const QString &id : _ecamCurrent.keys()) {
        const QString text = id.toLower();

        if (looksLikeBatteryText(text)) {
            out.append(id);
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
        FactValue value;
        Fact *fact = nullptr;
        QString text;

        if (id.startsWith(QStringLiteral("ecam."))) {
            value = _ecamCurrent.value(id);
            text = id.toLower();
        } else {
            value = _current.value(id);
            fact = _facts.value(id, nullptr);
            text = factSearchText(id, fact);
        }

        if (!value.ok || !value.numeric) {
            continue;
        }

        const double v = value.number;

        const bool batteryScoped =
            id.contains(QStringLiteral(".sns.bat."), Qt::CaseInsensitive)
            || id.contains(QStringLiteral(".battery."), Qt::CaseInsensitive)
            || id.contains(QStringLiteral(".bat"), Qt::CaseInsensitive)
            || id.contains(QStringLiteral("_bat"), Qt::CaseInsensitive)
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

QJsonObject QPilotTelemetry::ecamStatusJson() const
{
    QJsonObject out;
    out["url"] = _ecamUrl.toString();
    out["tracked_facts"] = _ecamCurrent.size();
    out["last_error"] = _ecamLastError;
    out["last_attempt_utc"] = _ecamLastAttemptMs > 0
        ? QDateTime::fromMSecsSinceEpoch(_ecamLastAttemptMs, Qt::UTC).toString(Qt::ISODateWithMs)
        : QString();
    out["last_ok_utc"] = _ecamLastOkMs > 0
        ? QDateTime::fromMSecsSinceEpoch(_ecamLastOkMs, Qt::UTC).toString(Qt::ISODateWithMs)
        : QString();
    out["ok"] = !_ecamCurrent.isEmpty() && _ecamLastError.isEmpty();

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
        ids = allIds();
    }

    QJsonArray requested;
    QJsonObject facts;

    for (const QString &id : ids) {
        requested.append(id);
        facts[id] = telemetryPayloadJson(id);
    }

    QJsonObject meta;
    meta["read_only"] = true;
    meta["raw_history_output"] = false;
    meta["window_s"] = QPilotStats::windowSeconds();
    meta["mandala_tracked_facts"] = _facts.size();
    meta["ecam_tracked_facts"] = _ecamCurrent.size();
    meta["derived_tracked_facts"] = _derivedCurrent.size();
    meta["total_tracked_facts"] = allIds().size();
    meta["correlations"] = "disabled_for_all_telemetry_collection";

    QJsonObject out;
    out["ok"] = true;
    out["server"] = "qpilot";
    out["mode"] = "mcp_mandala_facts_plus_ecam";
    out["requested_ids"] = requested;
    out["explicit_ids"] = explicitIds;
    out["tracked_ids"] = QJsonArray::fromStringList(allIds());
    out["mandala_tracked_ids"] = QJsonArray::fromStringList(trackedIds());
    out["ecam_tracked_ids"] = QJsonArray::fromStringList(ecamIds());
    out["derived_tracked_ids"] = QJsonArray::fromStringList(derivedIds());
    out["sample_period_ms"] = _timer.interval();
    out["ecam_sample_period_ms"] = _ecamTimer.interval();
    out["facts"] = facts;
    out["battery"] = batteryJson(batteryCells);
    out["ecam"] = ecamStatusJson();
    out["correlations"] = QJsonObject{
        {"total", QJsonObject{}},
        {"window", QJsonObject{}}
    };
    out["time_utc"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    out["meta"] = meta;

    return out;
}
