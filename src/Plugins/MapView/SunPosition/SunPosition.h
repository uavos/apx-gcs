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

#include <Fact/Fact.h>
#include <QtCore>
#include <QtPositioning>

class SunPosition : public Fact
{
    Q_OBJECT
    // names must not clash with child facts
    Q_PROPERTY(bool sunValid READ valid NOTIFY updated)
    Q_PROPERTY(double sunAzimuth READ azimuth NOTIFY updated)
    Q_PROPERTY(double sunAltitude READ altitude NOTIFY updated)
    Q_PROPERTY(double sunRelBearing READ relBearing NOTIFY updated)
    Q_PROPERTY(double sunRelElevation READ relElevation NOTIFY updated)

public:
    explicit SunPosition(Fact *parent = nullptr);

    enum TimeSource { TimeAuto = 0, TimeSystem };

    Fact *f_show;
    Fact *f_time;

    Fact *f_azimuth;
    Fact *f_altitude;
    Fact *f_rel_bearing;
    Fact *f_rel_elevation;
    Fact *f_utc;

    Fact *f_suncalc;

    bool valid() const { return m_valid; }
    double azimuth() const { return m_azimuth; }
    double altitude() const { return m_altitude; }
    double relBearing() const { return m_relBearing; }
    double relElevation() const { return m_relElevation; }

private:
    QTimer _timer;

    bool m_valid{};
    QGeoCoordinate m_coordinate;
    qint64 m_utc_ms{};
    double m_azimuth{};
    double m_altitude{};
    double m_relBearing{};
    double m_relElevation{};

    qint64 telemetryTime(bool *ok) const;

    static QString angleText(double v);

private slots:
    void update();
    void openSunCalc();

signals:
    void updated();
};
