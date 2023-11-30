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

#include "SimMod.h"

#include <MandalaBundles.h>
#include <MandalaMetaTree.h>

#include <QGeoCoordinate>

SimMod::SimMod(Fact *parent)
    : Fact(parent, "mod", tr("GPS Spoofing"), tr("Jamming patterns simulation"), Group)
{
    QString sect = tr("Velocity");
    f_vel_enb = new Fact(this, "vel_enb", tr("Enable"), tr("Modify Velocity"), Bool);
    f_vel_enb->setSection(sect);
    connect(f_vel_enb, &Fact::valueChanged, this, &SimMod::reset_vel);
    connect(f_vel_enb, &Fact::triggered, this, &SimMod::reset_vel);

    f_vel_period = new Fact(this,
                            "vel_period",
                            tr("Velocity period"),
                            tr("Period of velocity oscillation"),
                            Int);
    f_vel_period->setSection(sect);
    f_vel_period->setMin(100);
    f_vel_period->setMax(10 * 60 * 1000);
    f_vel_period->setUnits("ms");
    f_vel_period->setValue(1000);
    f_vel_period->setIncrement(100);

    f_vel_amplitude = new Fact(this,
                               "vel_amplitude",
                               tr("Velocity amplitude"),
                               tr("Amplitude of velocity oscillation"),
                               Int);
    f_vel_amplitude->setSection(sect);
    f_vel_amplitude->setMin(1);
    f_vel_amplitude->setMax(1000);
    f_vel_amplitude->setUnits("m/s");
    f_vel_amplitude->setValue(1);

    sect = tr("Position");
    f_pos_enb = new Fact(this, "pos_enb", tr("Enable"), tr("Modify Position"), Bool);
    f_pos_enb->setSection(sect);
    connect(f_pos_enb, &Fact::valueChanged, this, &SimMod::reset_pos);
    connect(f_pos_enb, &Fact::triggered, this, &SimMod::reset_pos);

    f_pos_shift = new Fact(this, "pos_shift", tr("Position shift"), tr("Shift of position"), Int);
    f_pos_shift->setSection(sect);
    f_pos_shift->setMin(-100000);
    f_pos_shift->setMax(100000);
    f_pos_shift->setUnits("m");
    f_pos_shift->setValue(0);
    f_pos_shift->setIncrement(10);

    f_hmsl_shift = new Fact(this, "hmsl_shift", tr("HMSL shift"), tr("Shift of HMSL"), Int);
    f_hmsl_shift->setSection(sect);
    f_hmsl_shift->setMin(-10000);
    f_hmsl_shift->setMax(10000);
    f_hmsl_shift->setUnits("m");
    f_hmsl_shift->setValue(0);
    f_hmsl_shift->setIncrement(1);
}

void SimMod::reset_vel()
{
    _vel_time.start();
    for (auto &v : _vel_phase) {
        v = QRandomGenerator::global()->bounded(1.);
    }
}

void SimMod::reset_pos()
{
    _pos_bearing = QRandomGenerator::global()->bounded(360.);
}

void SimMod::modify(mandala::bundle::sim_s *d)
{
    if (f_vel_enb->value().toBool()) {
        uint time_ms = _vel_time.elapsed();

        for (uint i = 0; i < 3; ++i) {
            double t = 2. * M_PI * time_ms / (double) f_vel_period->value().toInt();
            t += 2. * M_PI * _vel_phase[i];
            d->vel_ned[i] += f_vel_amplitude->value().toDouble() * std::sin(t);
        }
    }

    if (f_pos_enb->value().toBool()) {
        QGeoCoordinate pos(d->lat_deg, d->lon_deg, d->hmsl);
        pos = pos.atDistanceAndAzimuth(f_pos_shift->value().toDouble(),
                                       _pos_bearing,
                                       f_hmsl_shift->value().toDouble());

        d->lat_deg = pos.latitude();
        d->lon_deg = pos.longitude();
        d->hmsl = pos.altitude();
    }
}
