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

// The structure (forces/torques generation followed by rigid body
// equations of motion with fixed step integration) follows the PX4 SIH
// simulator (BSD-3-Clause, Copyright (c) 2019-2022 PX4 Development Team).
// The aerodynamics are replaced by a conventional linear coefficients
// model, blended into a flat plate beyond stall.

#include "Aircraft.h"
#include "Atmosphere.h"

namespace fdm {

static constexpr double kG = atmosphere::kG0;

Aircraft::Aircraft(const AircraftParams &params)
    : _p(params)
{}

void Aircraft::init(const Vec3 &pos_ned, double heading, double ground_alt)
{
    _ground_alt = ground_alt;
    _s = AircraftState();
    _ctr = Controls();

    // put the wheels on the ground
    double z_wheel = 0;
    for (const auto &w : _p.wheels)
        z_wheel = std::max(z_wheel, w.pos.z);

    _s.pos_ned = pos_ned;
    _s.pos_ned.z = pos_ned.z - z_wheel;
    _s.q = Quat::from_euler(0.0, 0.0, heading);
    _s.rpm = _p.rpm_idle;
    _s.acc_ned = Vec3(0, 0, -kG);
    _s.on_ground = true;

    // run a few steps to settle on the gear
    for (int i = 0; i < 1000; ++i)
        step(1.0 / 500.0);
}

// 1 while the flow is attached, 0 when fully stalled
static double stall_blend(double alpha, double alpha_s, double width)
{
    return 0.5 * (1.0 - std::tanh((std::fabs(alpha) - alpha_s) / (width / 4.0)));
}

void Aircraft::step(double dt)
{
    const AircraftParams &p = _p;
    AircraftState &s = _s;

    // --- atmosphere
    const double hmsl = _ground_alt - s.pos_ned.z;
    const auto atm = atmosphere::isa(hmsl);
    const double rho = atm.rho;

    // --- air data
    const Vec3 v_gnd_b = s.q.rotate_inv(s.vel_ned);
    const Vec3 v_air_b = s.q.rotate_inv(s.vel_ned - _wind_ned);
    const double V = v_air_b.norm();

    double alpha = 0, beta = 0;
    if (V > 0.5) {
        alpha = std::atan2(v_air_b.z, v_air_b.x);
        beta = std::asin(clamp(v_air_b.y / V, -1.0, 1.0));
    }
    const double qbar = 0.5 * rho * V * V;

    // --- controls
    const double da = clamp(_ctr.ail, -1.0, 1.0);
    const double de = clamp(_ctr.elv, -1.0, 1.0);
    const double dr = clamp(_ctr.rud + _ctr.rud_trim, -1.0, 1.0);
    const double df = clamp(_ctr.flaps, 0.0, 1.0);
    const double dsb = clamp(_ctr.airbrake, 0.0, 1.0);
    const double thr = clamp(_ctr.thr, 0.0, 1.0);
    const double brake = clamp(_ctr.brake, 0.0, 1.0);

    // --- nondimensional angular rates
    const double Vn = std::max(V, 1.0);
    const double p_hat = s.omega.x * p.b / (2.0 * Vn);
    const double q_hat = s.omega.y * p.c / (2.0 * Vn);
    const double r_hat = s.omega.z * p.b / (2.0 * Vn);

    // --- aerodynamic coefficients
    const double AR = p.b * p.b / p.S;
    const double sa = std::sin(alpha), ca = std::cos(alpha);
    const double sb = std::sin(beta), cb = std::cos(beta);

    const double alpha_s = p.alpha_stall - deg2rad(2.0) * df; // flaps reduce the stall angle
    const double sigma = stall_blend(alpha, alpha_s, p.stall_width);

    const double CL_lin = p.CL0 + p.CLa * alpha + p.CLq * q_hat + p.CLde * de + p.CLdf * df;
    const double CD_lin = p.CD0 + CL_lin * CL_lin / (kPi * p.e * AR) + p.CDdf * df + p.CDsb * dsb
                          + p.CDb * beta * beta;
    // flat plate (post stall)
    const double CL_fp = 1.1 * 2.0 * sa * ca;
    const double CD_fp = p.CD0 + 1.8 * sa * sa + p.CDdf * df + p.CDsb * dsb;

    const double CL = sigma * CL_lin + (1.0 - sigma) * CL_fp;
    const double CD = sigma * CD_lin + (1.0 - sigma) * CD_fp;
    const double CY = p.CYb * beta + p.CYp * p_hat + p.CYr * r_hat + p.CYdr * dr;
    const double Cl = p.Clb * beta + p.Clp * p_hat + p.Clr * r_hat + p.Clda * da + p.Cldr * dr;
    const double Cm = p.Cm0 + p.Cma * alpha + p.Cmq * q_hat + p.Cmde * de + p.Cmdf * df
                      + (1.0 - sigma) * (-0.2 * sa);
    const double Cn = p.Cnb * beta + p.Cnp * p_hat + p.Cnr * r_hat + p.Cndr * dr + p.Cnda * da;

    const double L = qbar * p.S * CL;
    const double D = qbar * p.S * CD;
    const double Y = qbar * p.S * CY;

    // wind axes (-D, Y, -L) -> body axes
    const Vec3 F_aero(-D * ca * cb - Y * ca * sb + L * sa,
                      -D * sb + Y * cb,
                      -D * sa * cb - Y * sa * sb - L * ca);
    const Vec3 M_aero(qbar * p.S * p.b * Cl, qbar * p.S * p.c * Cm, qbar * p.S * p.b * Cn);

    // --- propulsion: static thrust at low speed, power limited at high speed
    const double v0 = p.power / p.thrust_static;
    const double T = thr * p.power / (std::max(0.0, v_air_b.x) + v0);
    const Vec3 F_thr(T, 0.0, 0.0);

    const double rpm_sp = p.rpm_idle + (p.rpm_max - p.rpm_idle) * thr;
    s.rpm += (rpm_sp - s.rpm) * std::min(1.0, dt / p.engine_tau);

    // --- landing gear: spring-damper normal force, saturated viscous friction
    Vec3 F_gear, M_gear;
    bool on_ground = false;

    const double v_gnd = std::hypot(s.vel_ned.x, s.vel_ned.y);
    const double steer = clamp(_ctr.rud, -1.0, 1.0) * p.steer_max
                         / (1.0 + v_gnd / p.steer_fade_speed);

    for (const auto &w : p.wheels) {
        const Vec3 r = w.pos;
        const Vec3 pw = s.pos_ned + s.q.rotate(r);
        const double pen = pw.z; // ground plane is z=0, down positive
        if (pen <= 0)
            continue;
        on_ground = true;

        const Vec3 vw_b = v_gnd_b + s.omega.cross(r);
        const Vec3 vw = s.q.rotate(vw_b); // wheel velocity in NED

        double Fn = w.k * pen + w.c * vw.z;
        if (Fn < 0)
            Fn = 0;

        Vec3 F(0.0, 0.0, -Fn);

        // wheel rolling direction projected on the ground plane
        const double sw = w.steerable ? steer : 0.0;
        Vec3 fwd = s.q.rotate(Vec3(std::cos(sw), std::sin(sw), 0.0));
        fwd.z = 0;
        const double fn = fwd.norm();
        if (fn > 1e-3) {
            fwd = fwd / fn;
            const Vec3 side(-fwd.y, fwd.x, 0.0);
            const double v_f = vw.dot(fwd);
            const double v_s = vw.dot(side);
            const double mu_f = p.mu_roll + (w.brakes ? p.mu_brake * brake : 0.0);
            const double Ff = -clamp(v_f / p.slip_speed, -1.0, 1.0) * mu_f * Fn;
            const double Fs = -clamp(v_s / p.slip_speed, -1.0, 1.0) * p.mu_side * Fn;
            F += fwd * Ff + side * Fs;
        }

        const Vec3 Fb = s.q.rotate_inv(F);
        F_gear += Fb;
        M_gear += r.cross(Fb);
    }

    // --- equations of motion
    const Vec3 F_body = F_aero + F_thr + F_gear;
    const Vec3 M_body = M_aero + M_gear;

    const Vec3 f_ned = s.q.rotate(F_body) / p.mass; // specific force
    const Vec3 a_ned = f_ned + Vec3(0.0, 0.0, kG);

    // semi-implicit Euler
    s.vel_ned += a_ned * dt;
    s.pos_ned += s.vel_ned * dt;

    const Vec3 Iw = p.inertia.emul(s.omega);
    const Vec3 omega_dot = (M_body - s.omega.cross(Iw)).ediv(p.inertia);
    s.omega += omega_dot * dt;
    for (int i = 0; i < 3; ++i)
        s.omega[i] = clamp(s.omega[i], -4.0 * kPi, 4.0 * kPi);

    s.q.integrate(s.omega, dt);

    // --- derived outputs
    s.acc_ned = f_ned;
    s.hmsl = _ground_alt - s.pos_ned.z;
    s.tas = V;
    s.eas = V * std::sqrt(rho / atmosphere::kRho0);
    s.alpha = alpha;
    s.beta = beta;
    s.rho = rho;
    s.pressure = atm.pressure;
    s.air_temp = atm.temp_k - 273.15;
    s.on_ground = on_ground;
}

} // namespace fdm
