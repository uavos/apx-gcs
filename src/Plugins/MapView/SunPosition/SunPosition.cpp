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
#include "SunPosition.h"
#include "SunCalc.h"

#include <Fleet/Fleet.h>

#include <QDesktopServices>

SunPosition::SunPosition(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Sun position"),
           tr("Direction to the sun from the aircraft"),
           Group,
           "white-balance-sunny")
{
    f_show = new Fact(this,
                      "show",
                      tr("Show on map"),
                      tr("Show direction to the sun on the map"),
                      Bool | PersistentValue,
                      "eye");
    f_show->setDefaultValue(true);

    f_time = new Fact(this,
                      "time",
                      tr("Time source"),
                      tr("Auto: GPS time from telemetry or system clock"),
                      Enum | PersistentValue,
                      "clock-outline");
    f_time->setEnumStrings({"Auto", "System clock"});
    f_time->setDefaultValue(TimeAuto);

    const QString section = tr("Sun direction");

    f_azimuth = new Fact(this, "azimuth", tr("Azimuth"), tr("Direction from true North"));
    f_altitude = new Fact(this, "altitude", tr("Elevation"), tr("Angle above the horizon"));
    f_rel_bearing = new Fact(this,
                             "rel_bearing",
                             tr("Relative bearing"),
                             tr("Direction from the aircraft nose"));
    f_rel_elevation = new Fact(this,
                               "rel_elevation",
                               tr("Relative elevation"),
                               tr("Angle above the aircraft wings plane"));
    f_utc = new Fact(this, "utc", tr("Time"), tr("UTC time used for calculation"));

    for (auto f : {f_azimuth, f_altitude, f_rel_bearing, f_rel_elevation, f_utc})
        f->setSection(section);

    f_suncalc = new Fact(this,
                         "suncalc",
                         tr("Open suncalc.org"),
                         tr("Show current position and time on suncalc.org"),
                         Action,
                         "open-in-new");
    f_suncalc->setSection(section);
    f_suncalc->setEnabled(false);
    connect(f_suncalc, &Fact::triggered, this, &SunPosition::openSunCalc);

    connect(&_timer, &QTimer::timeout, this, &SunPosition::update);
    _timer.start(500);

    loadQml("qrc:/" PLUGIN_NAME "/SunPositionPlugin.qml");
}

qint64 SunPosition::telemetryTime(bool *ok) const
{
    *ok = false;
    auto unit = Fleet::instance()->current();
    if (!unit || !unit->f_mandala)
        return 0;

    // GPS time [s], contains CPU time when GPS is not available
    constexpr qint64 min_valid_time = 1577836800; // 2020-01-01
    const qint64 t = unit->f_mandala->fact(mandala::est::env::sys::time::uid)->value().toLongLong();
    if (t < min_valid_time)
        return 0;

    *ok = true;
    return t * 1000;
}

void SunPosition::update()
{
    auto unit = Fleet::instance()->current();
    const QGeoCoordinate c = unit ? unit->coordinate() : QGeoCoordinate();

    if (!c.isValid() || (c.latitude() == 0 && c.longitude() == 0)) {
        if (m_valid) {
            m_valid = false;
            for (auto f : {f_azimuth, f_altitude, f_rel_bearing, f_rel_elevation, f_utc})
                f->setValue(QVariant());
            f_suncalc->setEnabled(false);
            emit updated();
        }
        return;
    }

    bool gps = false;
    qint64 t = 0;
    if (f_time->value().toInt() == TimeAuto)
        t = telemetryTime(&gps);
    if (!gps)
        t = QDateTime::currentMSecsSinceEpoch();

    const auto sun = suncalc::sunPosition(t, c.latitude(), c.longitude());

    Mandala *m = unit->f_mandala;
    const double roll = m->fact(mandala::est::nav::att::roll::uid)->value().toDouble();
    const double pitch = m->fact(mandala::est::nav::att::pitch::uid)->value().toDouble();
    const double yaw = m->fact(mandala::est::nav::att::yaw::uid)->value().toDouble();
    const auto rel = suncalc::toBodyFrame(sun, roll, pitch, yaw);

    m_valid = true;
    m_coordinate = c;
    m_utc_ms = t;
    m_azimuth = sun.azimuth;
    m_altitude = sun.altitude;
    m_relBearing = rel.bearing;
    m_relElevation = rel.elevation;

    f_azimuth->setValue(angleText(m_azimuth));
    f_altitude->setValue(angleText(m_altitude));
    f_rel_bearing->setValue(QString("%1 %2")
                                .arg(angleText(std::abs(m_relBearing)))
                                .arg(m_relBearing < 0 ? tr("left") : tr("right")));
    f_rel_elevation->setValue(QString("%1 %2")
                                  .arg(angleText(std::abs(m_relElevation)))
                                  .arg(m_relElevation < 0 ? tr("below") : tr("above")));
    const QDateTime utc = QDateTime::fromMSecsSinceEpoch(t, QTimeZone::UTC);
    f_utc->setValue(
        QString("%1 (%2)").arg(utc.toString("yyyy-MM-dd HH:mm:ss"), gps ? "GPS" : tr("system")));
    f_suncalc->setEnabled(true);

    emit updated();
}

QString SunPosition::angleText(double v)
{
    return QString("%1°").arg(v, 0, 'f', 1);
}

void SunPosition::openSunCalc()
{
    if (!m_valid)
        return;

    // suncalc.org expects local time of the location,
    // assume the GCS is in the same time zone as the aircraft
    const QDateTime local = QDateTime::fromMSecsSinceEpoch(m_utc_ms).toLocalTime();

    const QString url = QString("https://www.suncalc.org/#/%1,%2,15/%3/%4/1/3")
                            .arg(m_coordinate.latitude(), 0, 'f', 6)
                            .arg(m_coordinate.longitude(), 0, 'f', 6)
                            .arg(local.toString("yyyy.MM.dd"))
                            .arg(local.toString("HH:mm"));
    QDesktopServices::openUrl(QUrl(url));
}
