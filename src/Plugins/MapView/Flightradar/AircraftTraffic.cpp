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
#include "AircraftTraffic.h"

AircraftTraffic::AircraftTraffic(Fact *parent)
    : Fact(parent)
{
    connect(&m_cleanupTimer, &QTimer::timeout, this, &AircraftTraffic::cleanupOldAircrafts);
    m_cleanupTimer.start(10000);
}

Aircraft *AircraftTraffic::getAircraft(const QString &callsign)
{
    auto ptr = m_aircrafts.value(callsign, nullptr);

    if (!ptr)
        return nullptr;
    return ptr;
}

QStringList AircraftTraffic::allCallsigns()
{
    return m_aircrafts.keys();
}

void AircraftTraffic::removeAircraft(const QString &callsign)
{
    if (m_aircrafts.contains(callsign)) {
        auto air = m_aircrafts.take(callsign);
        emit aircraftRemoved(callsign);
        air->deleteLater();
    }
}

void AircraftTraffic::updateFromAP(const GCS_TRAFFIC_REPORT_S &data)
{
    QString key = QString::fromLatin1(data.callsign).trimmed();
    if (key.isEmpty())
        return;

    Aircraft *air = m_aircrafts.value(key, nullptr);

    if (!air) {
        air = new Aircraft(this);
        air->setCallsign(key);
        m_aircrafts.insert(key, air);
    }

    if (!air) {
        return;
    }

    air->setIcaoAddress(data.ICAO_address);
    air->setLatitude(data.lat);
    air->setLongitude(data.lon);
    air->setHeading(data.heading);
    air->setAltitude(data.altitude);
    air->setSquawk(data.squawk);
    air->update();

    emit aircraftUpdated(key);
}

void AircraftTraffic::updateFromAP(const QString &callsign)
{
    Aircraft *air = m_aircrafts.value(callsign, nullptr);

    if (air) {
        const float dist = 100000;
        const float heading = air->heading();
        const float dlat = std::sin(heading * M_PI / 180) * dist;
        const float dlon = std::cos(heading * M_PI / 180) * dist;
        air->setLatitude(air->latitude() + dlat);
        air->setLongitude(air->longitude() + dlon);

        air->update();
        emit aircraftUpdated(callsign);
    }
}

void AircraftTraffic::cleanupOldAircrafts()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QList<QString> remove_list;

    for (auto it = m_aircrafts.cbegin(); it != m_aircrafts.cend(); ++it) {
        auto air = it.value();
        if (air && air->lastUpdate().msecsTo(now) > m_timeout_ms) {
            remove_list.append(it.key());
        }
    }

    for (const auto &key : remove_list) {
        emit aircraftTimeout(key);
        removeAircraft(key);
    }
}
