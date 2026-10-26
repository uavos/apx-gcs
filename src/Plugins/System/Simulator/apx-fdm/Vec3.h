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

// Minimal double precision vector/quaternion math for the FDM.

#include <algorithm>
#include <cmath>

namespace fdm {

constexpr double kPi = 3.14159265358979323846;

constexpr double deg2rad(double d)
{
    return d * kPi / 180.0;
}
constexpr double rad2deg(double r)
{
    return r * 180.0 / kPi;
}
inline double clamp(double v, double lo, double hi)
{
    return std::min(std::max(v, lo), hi);
}
// wrap angle to [0, 2pi)
inline double wrap_2pi(double a)
{
    a = std::fmod(a, 2.0 * kPi);
    if (a < 0)
        a += 2.0 * kPi;
    return a;
}
// wrap angle to [-pi, pi)
inline double wrap_pi(double a)
{
    a = wrap_2pi(a + kPi) - kPi;
    return a;
}

struct Vec3
{
    double x{}, y{}, z{};

    constexpr Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_)
        : x(x_)
        , y(y_)
        , z(z_)
    {}

    double operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    double &operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }

    Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }

    Vec3 &operator+=(const Vec3 &o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    Vec3 &operator-=(const Vec3 &o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }
    Vec3 &operator*=(double s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    double dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3 &o) const
    {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    double norm_sq() const { return dot(*this); }
    double norm() const { return std::sqrt(norm_sq()); }
    Vec3 normalized() const
    {
        const double n = norm();
        return n > 0 ? (*this) / n : Vec3();
    }
    // component-wise product
    Vec3 emul(const Vec3 &o) const { return {x * o.x, y * o.y, z * o.z}; }
    Vec3 ediv(const Vec3 &o) const { return {x / o.x, y / o.y, z / o.z}; }
};

inline Vec3 operator*(double s, const Vec3 &v)
{
    return v * s;
}

// Unit quaternion (w, x, y, z) describing the rotation body -> NED,
// i.e. v_ned = q.rotate(v_body).
struct Quat
{
    double w{1}, x{}, y{}, z{};

    constexpr Quat() = default;
    constexpr Quat(double w_, double x_, double y_, double z_)
        : w(w_)
        , x(x_)
        , y(y_)
        , z(z_)
    {}

    // ZYX (yaw, pitch, roll) Euler angles [rad]
    static Quat from_euler(double roll, double pitch, double yaw)
    {
        const double cr = std::cos(roll / 2), sr = std::sin(roll / 2);
        const double cp = std::cos(pitch / 2), sp = std::sin(pitch / 2);
        const double cy = std::cos(yaw / 2), sy = std::sin(yaw / 2);
        return {cr * cp * cy + sr * sp * sy,
                sr * cp * cy - cr * sp * sy,
                cr * sp * cy + sr * cp * sy,
                cr * cp * sy - sr * sp * cy};
    }

    Quat operator*(const Quat &o) const
    {
        return {w * o.w - x * o.x - y * o.y - z * o.z,
                w * o.x + x * o.w + y * o.z - z * o.y,
                w * o.y - x * o.z + y * o.w + z * o.x,
                w * o.z + x * o.y - y * o.x + z * o.w};
    }

    Quat conj() const { return {w, -x, -y, -z}; }

    void normalize()
    {
        const double n = std::sqrt(w * w + x * x + y * y + z * z);
        if (n > 0) {
            w /= n;
            x /= n;
            y /= n;
            z /= n;
        } else {
            *this = Quat();
        }
    }

    // body -> NED
    Vec3 rotate(const Vec3 &v) const
    {
        const Vec3 qv(x, y, z);
        const Vec3 t = qv.cross(v) * 2.0;
        return v + t * w + qv.cross(t);
    }
    // NED -> body
    Vec3 rotate_inv(const Vec3 &v) const { return conj().rotate(v); }

    // ZYX Euler angles [rad]: roll [-pi,pi], pitch [-pi/2,pi/2], yaw [-pi,pi]
    void to_euler(double &roll, double &pitch, double &yaw) const
    {
        roll = std::atan2(2.0 * (y * z + w * x), 1.0 - 2.0 * (x * x + y * y));
        pitch = std::asin(clamp(2.0 * (w * y - x * z), -1.0, 1.0));
        yaw = std::atan2(2.0 * (x * y + w * z), 1.0 - 2.0 * (y * y + z * z));
    }

    // integrate body angular rate [rad/s] over dt [s]
    void integrate(const Vec3 &omega, double dt)
    {
        const double angle = omega.norm() * dt;
        Quat dq;
        if (angle < 1e-9) {
            dq = Quat(1.0, omega.x * dt / 2, omega.y * dt / 2, omega.z * dt / 2);
        } else {
            const Vec3 axis = omega / omega.norm();
            const double s = std::sin(angle / 2);
            dq = Quat(std::cos(angle / 2), axis.x * s, axis.y * s, axis.z * s);
        }
        *this = (*this) * dq;
        normalize();
    }
};

} // namespace fdm
