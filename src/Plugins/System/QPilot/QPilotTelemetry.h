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
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>

class Fact;
class QNetworkReply;
class Unit;

class QPilotTelemetry : public QObject
{
    Q_OBJECT

public:
    explicit QPilotTelemetry(QObject *parent = nullptr);

    void start();
    void stop();

    QJsonObject toolJson(const QString &idsCsv, int batteryCells = 6) const;

    struct FactValue
    {
        bool ok = false;
        bool numeric = false;
        double number = 0.0;
        QString raw;
        QString error;
        qint64 timestampMs = 0;
    };

private:
    QTimer _timer;
    QTimer _ecamTimer;

    QNetworkAccessManager _network;
    QNetworkReply *_ecamReply = nullptr;

    QHash<QString, Fact *> _facts;
    QHash<QString, FactValue> _current;
    QHash<QString, FactValue> _ecamCurrent;
    QHash<QString, FactValue> _derivedCurrent;

    QPilotStats _stats;
    QPilotStats _ecamStats;
    QPilotStats _derivedStats;

    QUrl _ecamUrl;
    QString _ecamLastError;
    qint64 _ecamLastOkMs = 0;
    qint64 _ecamLastAttemptMs = 0;

    void bindUnit(Unit *unit);
    void sampleFacts();

    void fetchEcam();
    void handleEcamReply(QNetworkReply *reply);
    void ingestEcamXml(const QByteArray &xml, qint64 timestampMs);
    void updateDerivedHaps(qint64 timestampMs);

    void collectFactTree(Fact *fact, const QString &prefix, int depth = 0);
    void insertFact(const QString &id, Fact *fact);
    void insertAlias(const QString &id, Fact *fact);

    QStringList trackedIds() const;
    QStringList ecamIds() const;
    QStringList derivedIds() const;
    QStringList allIds() const;

    bool isTracked(const QString &id) const;
    bool isEcamTracked(const QString &id) const;
    bool isDerivedTracked(const QString &id) const;

    FactValue readFact(Fact *fact) const;

    QJsonObject factPayloadJson(const QString &id) const;
    QJsonObject ecamPayloadJson(const QString &id) const;
    QJsonObject derivedPayloadJson(const QString &id) const;
    QJsonObject telemetryPayloadJson(const QString &id) const;

    QStringList batteryCandidateIds() const;
    QJsonObject batteryJson(int cells) const;
    QJsonObject ecamStatusJson() const;

    static double estimateLipoSocPercent(double cellVoltage);
};
