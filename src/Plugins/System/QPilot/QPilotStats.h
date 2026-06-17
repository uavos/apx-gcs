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
#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

class QPilotStats
{
public:
    static int windowSeconds();

    void ingestBatch(const QMap<QString, double> &values, qint64 timestampMs);

    QJsonObject statsJson(const QString &id, double currentValue) const;
    QJsonObject correlationsJson(const QStringList &ids) const;

private:
    struct Sample
    {
        qint64 timestampMs = 0;
        double value = 0.0;
    };

    struct PairSample
    {
        qint64 timestampMs = 0;
        double x = 0.0;
        double y = 0.0;
    };

    struct OnlineFact
    {
        bool hasValue = false;
        int n = 0;

        qint64 firstMs = 0;
        qint64 lastMs = 0;
        qint64 prevMs = 0;

        double first = 0.0;
        double last = 0.0;
        double prev = 0.0;

        double min = 0.0;
        double max = 0.0;
        double mean = 0.0;
        double m2 = 0.0;

        double derivativeSum = 0.0;
        int derivativeN = 0;

        double sx = 0.0;
        double sy = 0.0;
        double sxx = 0.0;
        double sxy = 0.0;

        void ingest(double value, qint64 timestampMs);
        QJsonObject toJson() const;
    };

    struct WindowFact
    {
        QVector<Sample> samples;

        void ingest(double value, qint64 timestampMs);
        QJsonObject toJson() const;
    };

    struct FactSeries
    {
        OnlineFact total;
        WindowFact window;
    };

    struct OnlineCorr
    {
        int n = 0;

        double sx = 0.0;
        double sy = 0.0;
        double sxx = 0.0;
        double syy = 0.0;
        double sxy = 0.0;

        void ingest(double x, double y);
        QJsonObject toJson() const;
    };

    struct WindowCorr
    {
        QVector<PairSample> samples;

        void ingest(double x, double y, qint64 timestampMs);
        QJsonObject toJson() const;
    };

    struct PairSeries
    {
        QString a;
        QString b;
        OnlineCorr total;
        WindowCorr window;
    };

    QHash<QString, FactSeries> _facts;
    QHash<QString, PairSeries> _pairs;

    static constexpr int _windowSeconds = 30;

    static QString pairKey(const QString &a, const QString &b);
    static QJsonValue finiteJson(double value);
    static double seconds(qint64 firstMs, qint64 lastMs);
};
