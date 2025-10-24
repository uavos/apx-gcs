
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
#pragma once

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>

#include <Fact/Fact.h>

#include "Aircraft.h"

#pragma pack(1)
typedef struct
{
    uint32_t ICAO_address;
    int32_t lat;
    int32_t lon;
    uint16_t heading;
    int32_t altitude;
    uint16_t squawk;
    char callsign[9];
} __attribute__((packed)) GCS_TRAFFIC_REPORT_S;
#pragma pack()

class AircraftTraffic : public Fact
{
    Q_OBJECT
public:
    explicit AircraftTraffic(Fact *parent = nullptr);

    Q_INVOKABLE Aircraft *getAircraft(const QString &callsign);
    Q_INVOKABLE QStringList allCallsigns();
    Q_INVOKABLE void removeAircraft(const QString &callsign);
    Q_INVOKABLE void updateFromAP(const GCS_TRAFFIC_REPORT_S &data);

    void updateFromAP(const QString &callsign);

signals:
    Q_INVOKABLE void aircraftUpdated(const QString &callsign);
    Q_INVOKABLE void aircraftRemoved(const QString &callsign);
    Q_INVOKABLE void aircraftTimeout(const QString &callsign);

private slots:
    void cleanupOldAircrafts();

private:
    QMap<QString, Aircraft *> m_aircrafts;

    QTimer m_cleanupTimer;
    uint32_t m_timeout_ms = 30000;
};
