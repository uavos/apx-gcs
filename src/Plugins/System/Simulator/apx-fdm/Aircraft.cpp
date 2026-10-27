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
    _t = 0;

    _s.pos_ned = pos_ned;
    _s.acc_ned = Vec3(0, 0, -kG);
    _s.on_ground = true;

    if (_p.launcher) {
        // on the catapult rail, held until the launch command
        _s.pos_ned.z = pos_ned.z - _p.rail_height;
        _s.q = Quat::from_euler(0.0, _p.rail_pitch, heading);
        _s.rpm = _p.rpm_idle;
        _s.on_rail = true;
        return;
    }

    // put the wheels on the ground
    double z_wheel = 0;
    for (const auto &w : _p.wheels)
        z_wheel = std::max(z_wheel, w.pos.z);

    _s.pos_ned.z = pos_ned.z - z_wheel;
    _s.q = Quat::from_euler(0.0, 0.0, heading);
    _s.rpm = _p.type == AircraftType::Helicopter ? 0.0 : _p.rpm_idle;

    // run a few steps to settle on the gear
    for (int i = 0; i < 1000; ++i)
        step(1.0 / 500.0);
    _t = 0;
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

    _t += dt;

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

    // --- forces and moments in the body frame
    Vec3 F_body, M_body;
    bool on_ground = false;
    bool frozen = false;      // held on the launcher rail
    bool rail_guided = false; // pushed along the rail

    if (p.type == AircraftType::Helicopter)
        helicopter_forces(v_air_b, V, rho, dt, F_body, M_body);
    else
        fixed_wing_forces(v_air_b, V, rho, dt, F_body, M_body);

    if (p.launcher) {
        if (s.on_rail) {
            if (_ctr.launch >= 0.5) {
                s.on_rail = false;
                s.launched = true;
                s.t_launch = _t;
            } else {
                frozen = true;
            }
        }
        if (s.launched && _t - s.t_launch < p.launch_time) {
            // the rail pushes the aircraft and keeps its attitude
            F_body.x += p.mass * p.launch_acc;
            rail_guided = true;
        }
    }

    if (p.parachute) {
        if (_ctr.parachute >= 0.5)
            s.chute = true;
        if (s.chute) {
            const Vec3 v_att = v_air_b + s.omega.cross(p.chute_attach);
            const double va = v_att.norm();
            if (va > 0.1) {
                const Vec3 Fd = v_att * (-0.5 * rho * va * p.chute_cda);
                F_body += Fd;
                M_body += p.chute_attach.cross(Fd);
            }
            M_body -= s.omega * p.chute_damping;
        }
    }

    if (frozen) {
        // nothing moves, the IMU feels gravity only
        s.vel_ned = Vec3();
        s.omega = Vec3();
        s.acc_ned = Vec3(0.0, 0.0, -kG);
        on_ground = true;
    } else {
        if (!rail_guided)
            gear_forces(v_gnd_b, F_body, M_body, on_ground);

        // --- equations of motion
        const Vec3 f_ned = s.q.rotate(F_body) / p.mass; // specific force
        const Vec3 a_ned = f_ned + Vec3(0.0, 0.0, kG);

        // semi-implicit Euler
        s.vel_ned += a_ned * dt;
        s.pos_ned += s.vel_ned * dt;

        if (rail_guided) {
            s.omega = Vec3();
        } else {
            const Vec3 Iw = p.inertia.emul(s.omega);
            const Vec3 omega_dot = (M_body - s.omega.cross(Iw)).ediv(p.inertia);
            s.omega += omega_dot * dt;
            for (int i = 0; i < 3; ++i)
                s.omega[i] = clamp(s.omega[i], -4.0 * kPi, 4.0 * kPi);
            s.q.integrate(s.omega, dt);
        }

        s.acc_ned = f_ned;
    }

    // --- derived outputs
    s.hmsl = _ground_alt - s.pos_ned.z;
    s.agl = -s.pos_ned.z; // CG above the flat ground, like the X-Plane y_agl dataref
    s.tas = V;
    // the pitot reads the forward component only
    s.eas = std::max(0.0, v_air_b.x) * std::sqrt(rho / atmosphere::kRho0);
    s.alpha = alpha;
    s.beta = beta;
    s.rho = rho;
    s.pressure = atm.pressure;
    s.air_temp = atm.temp_k - 273.15;
    s.on_ground = on_ground;
}

// Linear aerodynamic coefficients blended into a flat plate beyond
// stall, propeller thrust limited by power at speed.
void Aircraft::fixed_wing_forces(
    const Vec3 &v_air_b, double V, double rho, double dt, Vec3 &F, Vec3 &M)
{
    const AircraftParams &p = _p;
    AircraftState &s = _s;

    const double qbar = 0.5 * rho * V * V;

    double a = 0, b = 0; // angle of attack, sideslip
    if (V > 0.5) {
        a = std::atan2(v_air_b.z, v_air_b.x);
        b = std::asin(clamp(v_air_b.y / V, -1.0, 1.0));
    }

    // --- controls
    const double da = clamp(_ctr.ail, -1.0, 1.0);
    const double de = clamp(_ctr.elv, -1.0, 1.0);
    const double dr = clamp(_ctr.rud + _ctr.rud_trim, -1.0, 1.0);
    const double df = clamp(_ctr.flaps, 0.0, 1.0);
    const double dsb = clamp(_ctr.airbrake, 0.0, 1.0);
    const double thr = clamp(_ctr.thr, 0.0, 1.0);

    // --- nondimensional angular rates
    const double Vn = std::max(V, 1.0);
    const double p_hat = s.omega.x * p.b / (2.0 * Vn);
    const double q_hat = s.omega.y * p.c / (2.0 * Vn);
    const double r_hat = s.omega.z * p.b / (2.0 * Vn);

    // --- aerodynamic coefficients
    const double AR = p.b * p.b / p.S;
    const double sa = std::sin(a), ca = std::cos(a);
    const double sb = std::sin(b), cb = std::cos(b);

    const double alpha_s = p.alpha_stall - deg2rad(2.0) * df; // flaps reduce the stall angle
    const double sigma = stall_blend(a, alpha_s, p.stall_width);

    const double CL_lin = p.CL0 + p.CLa * a + p.CLq * q_hat + p.CLde * de + p.CLdf * df;
    const double CD_lin = p.CD0 + CL_lin * CL_lin / (kPi * p.e * AR) + p.CDdf * df + p.CDsb * dsb
                          + p.CDb * b * b;
    // flat plate (post stall)
    const double CL_fp = 1.1 * 2.0 * sa * ca;
    const double CD_fp = p.CD0 + 1.8 * sa * sa + p.CDdf * df + p.CDsb * dsb;

    const double CL = sigma * CL_lin + (1.0 - sigma) * CL_fp;
    const double CD = sigma * CD_lin + (1.0 - sigma) * CD_fp;
    const double CY = p.CYb * b + p.CYp * p_hat + p.CYr * r_hat + p.CYdr * dr;
    const double Cl = p.Clb * b + p.Clp * p_hat + p.Clr * r_hat + p.Clda * da + p.Cldr * dr;
    const double Cm = p.Cm0 + p.Cma * a + p.Cmq * q_hat + p.Cmde * de + p.Cmdf * df
                      + (1.0 - sigma) * (-0.2 * sa);
    const double Cn = p.Cnb * b + p.Cnp * p_hat + p.Cnr * r_hat + p.Cndr * dr + p.Cnda * da;

    const double L = qbar * p.S * CL;
    const double D = qbar * p.S * CD;
    const double Y = qbar * p.S * CY;

    // wind axes (-D, Y, -L) -> body axes
    F += Vec3(-D * ca * cb - Y * ca * sb + L * sa,
              -D * sb + Y * cb,
              -D * sa * cb - Y * sa * sb - L * ca);
    M += Vec3(qbar * p.S * p.b * Cl, qbar * p.S * p.c * Cm, qbar * p.S * p.b * Cn);

    // --- propulsion
    const double v_fwd = std::max(0.0, v_air_b.x);
    double T;
    if (p.prop_pitch_speed > 0.0) {
        // fixed pitch propeller: thrust falls linearly with airspeed
        T = thr * p.thrust_static * std::max(0.0, 1.0 - v_fwd / p.prop_pitch_speed);
    } else {
        // static thrust at low speed, power limited at high speed
        const double v0 = p.power / p.thrust_static;
        T = thr * p.power / (v_fwd + v0);
    }
    F.x += T;

    const double rpm_sp = p.rpm_idle + (p.rpm_max - p.rpm_idle) * thr;
    s.rpm += (rpm_sp - s.rpm) * std::min(1.0, dt / p.engine_tau);
}

// Main rotor thrust from collective and inflow, thrust vector tilted by
// the cyclic with a hub spring, tail rotor against the reaction torque,
// fuselage drag. The engine speed follows the throttle with a load
// droop, the autopilot governor holds it at the nominal rpm.
void Aircraft::helicopter_forces(
    const Vec3 &v_air_b, double V, double rho, double dt, Vec3 &F, Vec3 &M)
{
    const AircraftParams::Rotor &r = _p.rotor;
    AircraftState &s = _s;

    const double ail = clamp(_ctr.ail, -1.0, 1.0);
    const double elv = clamp(_ctr.elv, -1.0, 1.0);
    const double rud = clamp(_ctr.rud, -1.0, 1.0);
    const double thr = clamp(_ctr.thr, 0.0, 1.0);
    const double theta = clamp(_ctr.collective, -15.0, 20.0); // [deg]

    // --- engine
    const double load = std::max(0.0, theta - r.theta0);
    const double rpm_sp = std::max(0.0, thr * (r.rpm_free - r.rpm_droop * load));
    s.rpm += (rpm_sp - s.rpm) * std::min(1.0, dt / r.rpm_tau);
    const double om2 = (s.rpm / r.rpm_nominal) * (s.rpm / r.rpm_nominal);

    // --- main rotor: vertical airspeed (body z, down positive) changes the inflow
    // the vertical airspeed (NED down positive) changes the inflow, forward
    // speed adds translational lift
    const Vec3 v_air_ned = s.q.rotate(v_air_b);
    const double v_h = std::hypot(v_air_ned.x, v_air_ned.y);
    const double theta_eff = theta + r.k_inflow * v_air_ned.z - r.theta0;
    const double tl = 1.0 + std::min(0.3, r.k_tl * v_h);
    const double T = r.kT * om2 * tl * std::max(0.0, theta_eff);

    const double d_lon = r.tilt_max * elv; // nose up: thrust tilts back
    const double d_lat = r.tilt_max * ail; // roll right: thrust tilts right
    const Vec3 F_rotor(-T * std::sin(d_lon),
                       T * std::sin(d_lat),
                       -T * std::cos(d_lon) * std::cos(d_lat));
    const Vec3 hub(0.0, 0.0, -r.hub_height);

    F += F_rotor;
    M += hub.cross(F_rotor) + Vec3(r.k_hub * ail * om2, r.k_hub * elv * om2, 0.0);

    // --- reaction torque and tail rotor (positive command: nose right)
    M.z -= r.k_torque * T;
    const double F_tail = -r.k_tail * rud * om2;
    F.y += F_tail;
    M.z += -r.tail_arm * F_tail;

    // --- vertical fin at the tail: weathercock stability in forward flight
    const double F_fin = -r.k_fin * v_air_b.y * V;
    F.y += F_fin;
    M.z += -r.tail_arm * F_fin;

    // --- fuselage drag and rotor damping
    F -= v_air_b * (0.5 * rho * V * r.cda);
    M -= r.damping.emul(s.omega);
}

// Landing gear: spring-damper normal force, saturated viscous friction
// (rolling resistance or skid friction, brakes, lateral grip), nose
// wheel steering.
void Aircraft::gear_forces(const Vec3 &v_gnd_b, Vec3 &F, Vec3 &M, bool &on_ground)
{
    const AircraftParams &p = _p;
    AircraftState &s = _s;

    const double brake = clamp(_ctr.brake, 0.0, 1.0);
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

        Vec3 Fw(0.0, 0.0, -Fn);

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
            Fw += fwd * Ff + side * Fs;
        }

        const Vec3 Fb = s.q.rotate_inv(Fw);
        F += Fb;
        M += r.cross(Fb);
    }
}

} // namespace fdm
