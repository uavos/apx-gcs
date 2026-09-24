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

#include <cstdint>

// Sun position calculator.
// Same method as used by https://www.suncalc.org (J. Meeus, Astronomical Algorithms):
// apparent solar coordinates, delta T correction, nutation, parallax
// and atmospheric refraction (Saemundsson, P=1010 hPa, T=10 C).
namespace suncalc {

struct Horizontal
{
    double azimuth;  // [deg] 0..360, from true North clockwise
    double altitude; // [deg] above horizon, refraction corrected
};

struct Relative
{
    double bearing;   // [deg] -180..180, from aircraft nose, positive to the right wing
    double elevation; // [deg] -90..90, above the aircraft XY (wings) plane
};

// utc_ms: unix time [ms], lat/lon: [deg]
Horizontal sunPosition(int64_t utc_ms, double lat, double lon);

// direction to the sun in aircraft body frame, roll/pitch/yaw: [deg]
Relative toBodyFrame(const Horizontal &sun, double roll, double pitch, double yaw);

// delta T = TT - UT [s], Espenak & Meeus polynomials
double deltaT(double year);

} // namespace suncalc
