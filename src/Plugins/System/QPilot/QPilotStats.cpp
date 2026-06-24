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
#include "QPilotStats.h"

#include <cmath>
#include <limits>

int QPilotStats::windowSeconds()
{
    return _windowSeconds;
}

QString QPilotStats::pairKey(const QString &a, const QString &b)
{
    return a <= b ? a + QChar(0x001f) + b : b + QChar(0x001f) + a;
}

QJsonValue QPilotStats::finiteJson(double value)
{
    return std::isfinite(value) ? QJsonValue(value) : QJsonValue(QJsonValue::Null);
}

double QPilotStats::seconds(qint64 firstMs, qint64 lastMs)
{
    return firstMs > 0 && lastMs > firstMs
        ? static_cast<double>(lastMs - firstMs) / 1000.0
        : 0.0;
}

void QPilotStats::OnlineFact::ingest(double value, qint64 timestampMs)
{
    if (!std::isfinite(value) || timestampMs <= 0) {
        return;
    }

    if (!hasValue) {
        hasValue = true;
        n = 1;

        firstMs = timestampMs;
        lastMs = timestampMs;
        prevMs = timestampMs;

        first = value;
        last = value;
        prev = value;

        min = value;
        max = value;
        mean = value;
        m2 = 0.0;

        sx = 0.0;
        sy = value;
        sxx = 0.0;
        sxy = 0.0;

        return;
    }

    const double dt = static_cast<double>(timestampMs - prevMs) / 1000.0;

    if (dt > 0.0) {
        derivativeSum += (value - prev) / dt;
        derivativeN++;
    }

    prev = value;
    prevMs = timestampMs;

    n++;
    last = value;
    lastMs = timestampMs;

    min = std::min(min, value);
    max = std::max(max, value);

    const double delta = value - mean;
    mean += delta / static_cast<double>(n);
    m2 += delta * (value - mean);

    const double t = static_cast<double>(timestampMs - firstMs) / 1000.0;

    sx += t;
    sy += value;
    sxx += t * t;
    sxy += t * value;
}

QJsonObject QPilotStats::OnlineFact::toJson() const
{
    QJsonObject out;
    out["n"] = n;

    if (!hasValue || n <= 0) {
        return out;
    }

    const double duration = QPilotStats::seconds(firstMs, lastMs);
    const double trendDelta = last - first;
    const double denom = static_cast<double>(n) * sxx - sx * sx;

    QJsonValue slope(QJsonValue::Null);

    if (n >= 2 && std::abs(denom) > std::numeric_limits<double>::epsilon()) {
        slope = QPilotStats::finiteJson((static_cast<double>(n) * sxy - sx * sy) / denom);
    }

    out["duration_s"] = duration;
    out["min"] = min;
    out["max"] = max;
    out["mean"] = mean;
    out["stdev"] = std::sqrt(m2 / static_cast<double>(n));
    out["trend_delta"] = trendDelta;
    out["avg_derivative_per_s"] = duration > 0.0 ? trendDelta / duration : 0.0;
    out["mean_step_derivative_per_s"] =
        derivativeN > 0 ? derivativeSum / static_cast<double>(derivativeN) : 0.0;
    out["trend_slope_per_s"] = slope;

    return out;
}

void QPilotStats::WindowFact::ingest(double value, qint64 timestampMs)
{
    if (!std::isfinite(value) || timestampMs <= 0) {
        return;
    }

    samples.append(Sample{timestampMs, value});

    const qint64 oldestAllowed = timestampMs - QPilotStats::_windowSeconds * 1000;

    while (!samples.isEmpty() && samples.first().timestampMs < oldestAllowed) {
        samples.removeFirst();
    }
}

QJsonObject QPilotStats::WindowFact::toJson() const
{
    QJsonObject out;
    out["window_s"] = QPilotStats::_windowSeconds;
    out["n"] = samples.size();

    if (samples.isEmpty()) {
        return out;
    }

    double min = samples.first().value;
    double max = samples.first().value;
    double mean = 0.0;
    double m2 = 0.0;
    double derivativeSum = 0.0;
    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;

    int derivativeN = 0;

    const qint64 firstMs = samples.first().timestampMs;
    const qint64 lastMs = samples.last().timestampMs;
    const double firstValue = samples.first().value;
    const double lastValue = samples.last().value;

    for (int i = 0; i < samples.size(); ++i) {
        const Sample &s = samples.at(i);

        min = std::min(min, s.value);
        max = std::max(max, s.value);

        const int n = i + 1;
        const double delta = s.value - mean;

        mean += delta / static_cast<double>(n);
        m2 += delta * (s.value - mean);

        const double t = static_cast<double>(s.timestampMs - firstMs) / 1000.0;

        sx += t;
        sy += s.value;
        sxx += t * t;
        sxy += t * s.value;

        if (i <= 0) {
            continue;
        }

        const Sample &p = samples.at(i - 1);
        const double dt = static_cast<double>(s.timestampMs - p.timestampMs) / 1000.0;

        if (dt > 0.0) {
            derivativeSum += (s.value - p.value) / dt;
            derivativeN++;
        }
    }

    const int n = samples.size();
    const double duration = QPilotStats::seconds(firstMs, lastMs);
    const double trendDelta = lastValue - firstValue;
    const double denom = static_cast<double>(n) * sxx - sx * sx;

    QJsonValue slope(QJsonValue::Null);

    if (n >= 2 && std::abs(denom) > std::numeric_limits<double>::epsilon()) {
        slope = QPilotStats::finiteJson((static_cast<double>(n) * sxy - sx * sy) / denom);
    }

    out["duration_s"] = duration;
    out["min"] = min;
    out["max"] = max;
    out["mean"] = mean;
    out["stdev"] = std::sqrt(m2 / static_cast<double>(n));
    out["trend_delta"] = trendDelta;
    out["avg_derivative_per_s"] = duration > 0.0 ? trendDelta / duration : 0.0;
    out["mean_step_derivative_per_s"] =
        derivativeN > 0 ? derivativeSum / static_cast<double>(derivativeN) : 0.0;
    out["trend_slope_per_s"] = slope;

    return out;
}

void QPilotStats::OnlineCorr::ingest(double x, double y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }

    n++;

    sx += x;
    sy += y;
    sxx += x * x;
    syy += y * y;
    sxy += x * y;
}

QJsonObject QPilotStats::OnlineCorr::toJson() const
{
    QJsonObject out;
    out["n"] = n;

    if (n < 2) {
        out["correlation"] = QJsonValue(QJsonValue::Null);
        return out;
    }

    const double dn = static_cast<double>(n);
    const double numerator = dn * sxy - sx * sy;
    const double denomX = dn * sxx - sx * sx;
    const double denomY = dn * syy - sy * sy;
    const double denom = std::sqrt(denomX * denomY);

    out["correlation"] =
        denom > std::numeric_limits<double>::epsilon() && std::isfinite(denom)
        ? QJsonValue(numerator / denom)
        : QJsonValue(QJsonValue::Null);

    return out;
}

void QPilotStats::WindowCorr::ingest(double x, double y, qint64 timestampMs)
{
    if (!std::isfinite(x) || !std::isfinite(y) || timestampMs <= 0) {
        return;
    }

    samples.append(PairSample{timestampMs, x, y});

    const qint64 oldestAllowed = timestampMs - QPilotStats::_windowSeconds * 1000;

    while (!samples.isEmpty() && samples.first().timestampMs < oldestAllowed) {
        samples.removeFirst();
    }
}

QJsonObject QPilotStats::WindowCorr::toJson() const
{
    QJsonObject out;
    out["window_s"] = QPilotStats::_windowSeconds;
    out["n"] = samples.size();

    if (samples.size() < 2) {
        out["correlation"] = QJsonValue(QJsonValue::Null);
        return out;
    }

    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double syy = 0.0;
    double sxy = 0.0;

    for (const PairSample &s : samples) {
        sx += s.x;
        sy += s.y;
        sxx += s.x * s.x;
        syy += s.y * s.y;
        sxy += s.x * s.y;
    }

    const double n = static_cast<double>(samples.size());
    const double numerator = n * sxy - sx * sy;
    const double denomX = n * sxx - sx * sx;
    const double denomY = n * syy - sy * sy;
    const double denom = std::sqrt(denomX * denomY);

    out["correlation"] =
        denom > std::numeric_limits<double>::epsilon() && std::isfinite(denom)
        ? QJsonValue(numerator / denom)
        : QJsonValue(QJsonValue::Null);

    return out;
}

void QPilotStats::ingestBatch(const QMap<QString, double> &values,
                              qint64 timestampMs,
                              bool collectCorrelations)
{
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        FactSeries &series = _facts[it.key()];

        series.total.ingest(it.value(), timestampMs);
        series.window.ingest(it.value(), timestampMs);
    }

    if (!collectCorrelations) {
        return;
    }

    const QStringList ids = values.keys();

    for (int i = 0; i < ids.size(); ++i) {
        for (int j = i + 1; j < ids.size(); ++j) {
            const QString a = ids.at(i);
            const QString b = ids.at(j);
            PairSeries &pair = _pairs[pairKey(a, b)];

            if (pair.a.isEmpty()) {
                pair.a = a;
                pair.b = b;
            }

            pair.total.ingest(values.value(a), values.value(b));
            pair.window.ingest(values.value(a), values.value(b), timestampMs);
        }
    }
}

QJsonObject QPilotStats::statsJson(const QString &id, double currentValue) const
{
    QJsonObject out;
    out["value"] = currentValue;

    const auto it = _facts.constFind(id);

    if (it == _facts.constEnd()) {
        QJsonObject window;
        window["window_s"] = _windowSeconds;

        out["stats_total"] = QJsonObject();
        out["stats_window"] = window;

        return out;
    }

    out["stats_total"] = it.value().total.toJson();
    out["stats_window"] = it.value().window.toJson();

    return out;
}

QJsonObject QPilotStats::correlationsJson(const QStringList &ids) const
{
    QStringList cleanIds = ids;
    cleanIds.removeDuplicates();

    QJsonObject total;
    QJsonObject window;

    for (int i = 0; i < cleanIds.size(); ++i) {
        for (int j = i + 1; j < cleanIds.size(); ++j) {
            const QString a = cleanIds.at(i);
            const QString b = cleanIds.at(j);
            const auto it = _pairs.constFind(pairKey(a, b));

            if (it == _pairs.constEnd()) {
                continue;
            }

            const QString name = a + QStringLiteral("~") + b;

            total[name] = it.value().total.toJson();
            window[name] = it.value().window.toJson();
        }
    }

    return QJsonObject{
        {"total", total},
        {"window", window}
    };
}
