/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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

#include <algorithm>
#include <cmath>

// International Standard Atmosphere (troposphere)
namespace fdm {
namespace atmosphere {

constexpr double kT0 = 288.15;    // sea level temperature [K]
constexpr double kP0 = 101325.0;  // sea level pressure [Pa]
constexpr double kRho0 = 1.225;   // sea level density [kg/m^3]
constexpr double kLapse = 0.0065; // temperature lapse rate [K/m]
constexpr double kR = 287.05287;  // specific gas constant of air [J/kg/K]
constexpr double kG0 = 9.80665;   // standard gravity [m/s^2]

struct State
{
    double temp_k;   // [K]
    double pressure; // [Pa]
    double rho;      // [kg/m^3]
};

inline State isa(double hmsl)
{
    hmsl = std::min(std::max(hmsl, -1000.0), 11000.0);
    State s;
    s.temp_k = kT0 - kLapse * hmsl;
    s.pressure = kP0 * std::pow(s.temp_k / kT0, kG0 / (kLapse * kR));
    s.rho = s.pressure / (kR * s.temp_k);
    return s;
}

} // namespace atmosphere
} // namespace fdm
