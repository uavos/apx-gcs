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

#include "QPilotStats.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>

class Fact;
class Unit;

class QPilotTelemetry : public QObject
{
    Q_OBJECT

public:
    explicit QPilotTelemetry(QObject *parent = nullptr);

    void start();
    void stop();

    QJsonObject toolJson(const QString &idsCsv) const;

private:
    struct FactValue
    {
        bool ok = false;
        bool numeric = false;
        double number = 0.0;
        QString raw;
        QString error;
        qint64 timestampMs = 0;
    };

    QTimer _timer;
    QHash<QString, Fact *> _facts;
    QHash<QString, FactValue> _current;
    QPilotStats _stats;

    void bindUnit(Unit *unit);
    void sampleFacts();

    QStringList trackedIds() const;
    bool isTracked(const QString &id) const;

    FactValue readFact(Fact *fact) const;
    QJsonObject factPayloadJson(const QString &id) const;
};
