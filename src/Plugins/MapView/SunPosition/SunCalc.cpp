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
#include "SunCalc.h"

#include <cmath>

namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double D2R = PI / 180.0;
constexpr double R2D = 180.0 / PI;
constexpr double ARCSEC = D2R / 3600.0;

constexpr double JD_UNIX_EPOCH = 2440587.5;
constexpr double JD_J2000 = 2451545.0;

constexpr double AU = 149597870700.0;      // [m]
constexpr double EARTH_RADIUS = 6378137.0; // [m]

double wrap360(double v)
{
    v = std::fmod(v, 360.0);
    return v < 0 ? v + 360.0 : v;
}

double wrap180(double v)
{
    v = wrap360(v);
    return v > 180.0 ? v - 360.0 : v;
}

} // namespace

double suncalc::deltaT(double y)
{
    double t;
    if (y >= 1900 && y < 1920) {
        t = y - 1900;
        return -2.79 + t * (1.494119 + t * (-0.0598939 + t * (0.0061966 - t * 0.000197)));
    }
    if (y >= 1920 && y < 1941) {
        t = y - 1920;
        return 21.20 + t * (0.84493 + t * (-0.0761 + t * 0.0020936));
    }
    if (y >= 1941 && y < 1961) {
        t = y - 1950;
        return 29.07 + t * (0.407 + t * (-1.0 / 233 + t / 2547));
    }
    if (y >= 1961 && y < 1986) {
        t = y - 1975;
        return 45.45 + t * (1.067 + t * (-1.0 / 260 - t / 718));
    }
    if (y >= 1986 && y < 2005) {
        t = y - 2000;
        return 63.86
               + t
                     * (0.3345
                        + t * (-0.060374 + t * (0.0017275 + t * (0.000651814 + t * 0.00002373599))));
    }
    if (y >= 2005 && y < 2050) {
        t = y - 2000;
        return 62.92 + t * (0.32217 + t * 0.005589);
    }
    const double u = (y - 1820) / 100;
    if (y >= 2050 && y < 2150)
        return -20 + 32 * u * u - 0.5628 * (2150 - y);
    return -20 + 32 * u * u;
}

suncalc::Horizontal suncalc::sunPosition(int64_t utc_ms, double lat, double lon)
{
    const double jd = JD_UNIX_EPOCH + utc_ms / 86400000.0;
    const double jde = jd + deltaT(2000.0 + (jd - JD_J2000) / 365.25) / 86400.0;
    const double T = (jde - JD_J2000) / 36525.0;

    // geometric mean longitude and mean anomaly of the Sun [deg]
    const double L0 = 280.46646 + T * (36000.76983 + T * 0.0003032);
    const double M = 357.52911 + T * (35999.05029 - T * 0.0001537);
    const double Mr = M * D2R;

    // equation of center
    const double C = (1.914602 - T * (0.004817 + T * 0.000014)) * std::sin(Mr)
                     + (0.019993 - T * 0.000101) * std::sin(2 * Mr) + 0.000289 * std::sin(3 * Mr);

    // Earth-Sun distance [AU]
    const double e = 0.016708634 - T * (0.000042037 + T * 0.0000001267);
    const double R = 1.000001018 * (1 - e * e) / (1 + e * std::cos((M + C) * D2R));

    // nutation in longitude and obliquity
    const double omega = (125.04452 - T * (1934.136261 - T * (0.0020708 + T / 450000.0))) * D2R;
    const double Ls = (280.4665 + 36000.7698 * T) * D2R;
    const double Lm = (218.3165 + 481267.8813 * T) * D2R;
    const double dpsi = (-17.20 * std::sin(omega) - 1.32 * std::sin(2 * Ls)
                         - 0.23 * std::sin(2 * Lm) + 0.21 * std::sin(2 * omega))
                        * ARCSEC;
    const double deps = (9.20 * std::cos(omega) + 0.57 * std::cos(2 * Ls) + 0.10 * std::cos(2 * Lm)
                         - 0.09 * std::cos(2 * omega))
                        * ARCSEC;

    // apparent longitude of the Sun (nutation and aberration)
    const double omega_s = (125.04 - 1934.136 * T) * D2R;
    const double lambda = (L0 + C - 0.00569 - 0.00478 * std::sin(omega_s)) * D2R;

    // obliquity of the ecliptic (Laskar) [arcsec]
    static constexpr double k[]
        = {-4680.93, -1.55, 1999.25, -51.38, -249.67, -39.05, 7.12, 27.87, 5.79, 2.45};
    const double U = T / 100.0;
    double eps0_sec = 0;
    for (int i = 9; i >= 0; --i)
        eps0_sec = (eps0_sec + k[i]) * U;
    eps0_sec += 21.448;
    const double eps = (23.0 + 26.0 / 60.0) * D2R + eps0_sec * ARCSEC + deps;
    const double eps_app = eps + 0.00256 * D2R * std::cos(omega_s);

    // apparent right ascension and declination
    const double ra = std::atan2(std::cos(eps_app) * std::sin(lambda), std::cos(lambda));
    const double dec = std::asin(std::sin(eps_app) * std::sin(lambda));

    // apparent sidereal time at Greenwich
    const double gmst = 280.46061837 + 360.98564736629 * (jd - JD_J2000)
                        + T * T * (0.000387933 - T / 38710000.0);
    const double gast = wrap360(gmst) * D2R + dpsi * std::cos(eps);

    // local hour angle
    const double H = gast + lon * D2R - ra;
    const double phi = lat * D2R;

    // azimuth from South, altitude
    const double az = std::atan2(std::sin(H),
                                 std::cos(H) * std::sin(phi) - std::tan(dec) * std::cos(phi));
    double alt = std::asin(std::sin(phi) * std::sin(dec)
                           + std::cos(phi) * std::cos(dec) * std::cos(H));

    // parallax
    alt -= std::asin(EARTH_RADIUS / (R * AU) * std::cos(alt));

    // refraction [arcmin], the formula is singular below the horizon
    double h = alt * R2D;
    if (h > -1.0)
        h += 1.02 / std::tan((h + 10.3 / (h + 5.11)) * D2R) / 60.0;

    return {wrap360(az * R2D + 180.0), h};
}

suncalc::Relative suncalc::toBodyFrame(const Horizontal &sun, double roll, double pitch, double yaw)
{
    const double az = sun.azimuth * D2R;
    const double el = sun.altitude * D2R;

    // unit vector to the sun, NED frame
    const double n = std::cos(el) * std::cos(az);
    const double e = std::cos(el) * std::sin(az);
    const double d = -std::sin(el);

    // rotate to body frame (yaw, pitch, roll)
    const double sy = std::sin(yaw * D2R), cy = std::cos(yaw * D2R);
    const double sp = std::sin(pitch * D2R), cp = std::cos(pitch * D2R);
    const double sr = std::sin(roll * D2R), cr = std::cos(roll * D2R);

    const double x1 = cy * n + sy * e;
    const double y1 = -sy * n + cy * e;
    const double z1 = d;

    const double x2 = cp * x1 - sp * z1;
    const double z2 = sp * x1 + cp * z1;

    const double x = x2;
    const double y = cr * y1 + sr * z2;
    const double z = -sr * y1 + cr * z2;

    return {wrap180(std::atan2(y, x) * R2D), std::asin(std::fmax(-1.0, std::fmin(1.0, -z))) * R2D};
}
