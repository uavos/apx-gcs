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

#include <MandalaBundles.h>

class SimMod : public Fact
{
    Q_OBJECT

public:
    explicit SimMod(Fact *parent = nullptr);

    void modify(mandala::bundle::sim_s *d);

private:
    Fact *f_vel_enb;
    Fact *f_vel_period;
    Fact *f_vel_amplitude;

    Fact *f_pos_enb;
    Fact *f_pos_shift;
    Fact *f_hmsl_shift;

    // model
    double _vel_phase[3] = {}; // phase shift [0..1]
    QElapsedTimer _vel_time;

    double _pos_bearing = {}; // pos shift bearing [deg]

private slots:
    void reset_vel();
    void reset_pos();
};
