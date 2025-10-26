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

#include <QRandomGenerator>

#include <App/AppGcs.h>
#include <Fact/Fact.h>
#include <Fleet/Unit.h>

#include "AircraftTraffic.h"

//#define PINGRX_SIMULATION

class PingRX : public Fact
{
    Q_OBJECT

public:
    explicit PingRX(Fact *parent,
                    const QString &name,
                    const QString &title,
                    const QString &descr,
                    const QString &icon);

private:
    Fact *f_enabled;
    Fact *f_vcpid;

    PData *_pdata{};

    AircraftTraffic *_at{};

    QObject *qml;

private slots:
    void onCurrentUnitChanged();
    void onPdataSerialData(quint8 portID, QByteArray data);
    void updateStatus();

private:
#ifdef PINGRX_SIMULATION
    const uint32_t count_test_uav{100};
    QTimer m_testTimer;

    const double m_minLat = -90.0;
    const double m_maxLat = 90.0;
    const double m_minLon = -180.0;
    const double m_maxLon = 180.0;

    const int m_minAlt = 0;
    const int m_maxAlt = 12000;

    GCS_TRAFFIC_REPORT_S make_random_report()
    {
        GCS_TRAFFIC_REPORT_S r{};

        r.ICAO_address = randomICAO();

        const double lat = m_minLat
                           + (m_maxLat - m_minLat) * QRandomGenerator::global()->generateDouble();
        const double lon = m_minLon
                           + (m_maxLon - m_minLon) * QRandomGenerator::global()->generateDouble();

        r.lat = lat * 1e7;
        r.lon = lon * 1e7;

        r.heading = randomHeading();
        r.altitude = randomAltitude(m_minAlt, m_maxAlt);
        r.squawk = randomSquawk();

        randomCallsign(r.callsign);

        return r;
    }

    void create_test_uav()
    {
        for (uint32_t i = 0; i < count_test_uav; i++) {
            _at->updateFromAP(make_random_report());
        }
    }
    void update_test_uav()
    {
        QStringList callsigns = _at->allCallsigns();
        for (uint32_t i = 0; i < callsigns.size(); i++) {
            Aircraft *aircraft = _at->getAircraft(callsigns.at(i));
            if (aircraft) {
                _at->updateSimData(callsigns.at(i));
            }
        }
    }

    uint32_t randomICAO()
    {
        return static_cast<uint32_t>(QRandomGenerator::global()->generate() & 0x00FFFFFFu);
    }

    uint16_t randomHeading()
    {
        return static_cast<uint16_t>(QRandomGenerator::global()->bounded(360u) * 1e2);
    }

    int32_t randomAltitude(int minAlt, int maxAlt)
    {
        return static_cast<int32_t>(QRandomGenerator::global()->bounded(minAlt, maxAlt + 1) * 1e3);
    }

    uint16_t randomSquawk()
    {
        return static_cast<uint16_t>(QRandomGenerator::global()->bounded(0, 7778));
    }

    void randomCallsign(char out[9])
    {
        static const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        constexpr int alphaSize = sizeof(alpha) - 1;

        int len = QRandomGenerator::global()->bounded(3, 9);

        for (int i = 0; i < 8; ++i) {
            if (i < len) {
                int idx = QRandomGenerator::global()->bounded(alphaSize);
                out[i] = alpha[idx];
            } else {
                out[i] = ' ';
            }
        }
        out[8] = '\0';
    }

    int32_t degToInt32(double deg) { return deg * 1e7; }
#endif // PINGRX_SIMULATION
};
