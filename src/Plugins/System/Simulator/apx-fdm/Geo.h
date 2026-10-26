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

#include "Vec3.h"

namespace fdm {

// Local flat-earth tangent plane (NED) attached to the start point.
// Good enough for the few tens of kilometres a SIL mission covers.
class GeoOrigin
{
public:
    void init(double lat_deg, double lon_deg, double alt_msl)
    {
        _lat_deg = lat_deg;
        _lon_deg = lon_deg;
        _alt = alt_msl;

        const double lat = deg2rad(lat_deg);
        const double sin_lat = std::sin(lat);
        const double den = 1.0 - kE2 * sin_lat * sin_lat;
        _r_m = kA * (1.0 - kE2) / (den * std::sqrt(den)); // meridional radius
        const double r_n = kA / std::sqrt(den);           // normal (prime vertical) radius
        _r_n_cos = r_n * std::cos(lat);
    }

    double lat_deg() const { return _lat_deg; }
    double lon_deg() const { return _lon_deg; }
    double alt() const { return _alt; }

    // NED position [m] -> geodetic coordinates
    void to_llh(const Vec3 &ned, double &lat_deg, double &lon_deg, double &hmsl) const
    {
        lat_deg = _lat_deg + rad2deg(ned.x / _r_m);
        lon_deg = _lon_deg + rad2deg(ned.y / _r_n_cos);
        hmsl = _alt - ned.z;
    }

private:
    static constexpr double kA = 6378137.0;         // WGS84 semi-major axis
    static constexpr double kE2 = 0.00669437999014; // WGS84 first eccentricity squared

    double _lat_deg{}, _lon_deg{}, _alt{};
    double _r_m{kA}, _r_n_cos{kA};
};

} // namespace fdm
