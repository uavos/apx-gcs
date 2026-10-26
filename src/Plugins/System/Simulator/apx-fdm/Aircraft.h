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

// Control inputs of the model, all in the units the SIL node sends
// for the corresponding X-Plane datarefs:
//  - surfaces -1..1 (positive: roll right, nose up, nose right)
//  - throttle, flaps, brake, airbrake 0..1
struct Controls
{
    double ail{};      // sim/joystick/yoke_roll_ratio
    double elv{};      // sim/joystick/yoke_pitch_ratio
    double rud{};      // sim/joystick/yoke_heading_ratio (rudder + nose wheel steering)
    double rud_trim{}; // sim/flightmodel/controls/rud_trim (rudder only)
    double thr{};      // sim/flightmodel/engine/ENGN_thro_use[0]
    double flaps{};    // sim/flightmodel/controls/flaprqst
    double brake{};    // sim/flightmodel/controls/parkbrake
    double airbrake{}; // sim/flightmodel/controls/sbrkrqst
};

struct Wheel
{
    Vec3 pos;       // body frame position from CG [m] (x fwd, y right, z down)
    double k;       // spring stiffness [N/m]
    double c;       // damping [N*s/m]
    bool steerable; // follows the rudder/steering command
    bool brakes;    // affected by the brake command
};

// Rigid body 6-DOF fixed-wing aircraft with linear aerodynamic
// coefficients (stability and control derivatives), a power-limited
// propeller and three-wheel landing gear.
// Default values describe a ~1200 kg MALE class UAV, roughly the S1C
// model used with X-Plane: stalls at ~29 m/s clean, cruises at ~40 m/s.
struct AircraftParams
{
    // mass properties
    double mass{1200.0};                  // [kg]
    Vec3 inertia{2200.0, 1500.0, 3300.0}; // Ixx, Iyy, Izz [kg*m^2] (products of inertia ignored)

    // geometry
    double S{14.0};  // wing area [m^2]
    double b{16.0};  // wing span [m]
    double c{0.875}; // mean aerodynamic chord [m]

    // lift: CL = CL0 + CLa*alpha + CLq*q^ + CLde*de + CLdf*df
    double CL0{0.4};
    double CLa{5.6}; // 1/rad
    double CLq{6.0};
    double CLde{-0.15};                // per unit elevator command (pull = tail down force)
    double CLdf{0.8};                  // per unit flaps
    double alpha_stall{deg2rad(15.0)}; // centre of the stall transition
    double stall_width{deg2rad(4.0)};  // width of the stall transition

    // drag: CD = CD0 + CL^2/(pi*e*AR) + CDdf*df + CDsb*dsb + CDb*beta^2
    double CD0{0.030};
    double e{0.85}; // Oswald efficiency
    double CDdf{0.08};
    double CDsb{0.12};
    double CDb{0.3};

    // side force
    double CYb{-0.6};
    double CYp{0.0};
    double CYr{0.2};
    double CYdr{0.12};

    // roll moment
    double Clb{-0.08};
    double Clp{-0.55};
    double Clr{0.15};
    double Clda{0.10};
    double Cldr{0.01};

    // pitch moment
    double Cm0{0.04};
    double Cma{-1.0};
    double Cmq{-18.0};
    double Cmde{0.65};
    double Cmdf{-0.12};

    // yaw moment
    double Cnb{0.09};
    double Cnp{-0.04};
    double Cnr{-0.18};
    double Cndr{0.07};
    double Cnda{-0.005};

    // propulsion: T = thr * power / (V + power/thrust_static)
    double thrust_static{5500.0}; // [N]
    double power{150000.0};       // propeller shaft power * efficiency [W]
    double rpm_idle{1500.0};
    double rpm_max{6500.0};
    double engine_tau{0.4}; // engine speed time constant [s]

    // ground handling
    double steer_max{deg2rad(20.0)}; // nose wheel deflection at full command
    double steer_fade_speed{20.0};   // steering authority halves at this ground speed [m/s]
    double mu_roll{0.03};            // rolling resistance
    double mu_brake{0.6};            // additional friction at full brake
    double mu_side{0.9};             // lateral tyre friction
    double slip_speed{0.3};          // friction saturates above this slip speed [m/s]

    Wheel wheels[3]{
        {{2.3, 0.0, 1.2}, 60e3, 5e3, true, false},      // nose
        {{-0.45, -1.45, 1.2}, 100e3, 8e3, false, true}, // left main
        {{-0.45, 1.45, 1.2}, 100e3, 8e3, false, true},  // right main
    };
};

struct AircraftState
{
    Vec3 pos_ned; // position relative to origin [m]
    Vec3 vel_ned; // velocity [m/s]
    Quat q;       // attitude body -> NED
    Vec3 omega;   // body angular rates [rad/s]
    Vec3 acc_ned; // specific force in NED [m/s^2] (at rest: 0,0,-g)
    double rpm{}; // engine speed

    // derived
    double hmsl{};     // [m]
    double tas{};      // true airspeed [m/s]
    double eas{};      // equivalent airspeed [m/s]
    double alpha{};    // angle of attack [rad]
    double beta{};     // sideslip [rad]
    double rho{};      // air density [kg/m^3]
    double pressure{}; // static pressure [Pa]
    double air_temp{}; // [C]
    bool on_ground{true};
};

class Aircraft
{
public:
    explicit Aircraft(const AircraftParams &params = {});

    // Place the aircraft on the ground at NED position (z is ground level)
    // with the given heading [rad]. ground_alt is the MSL altitude of the
    // (flat) ground.
    void init(const Vec3 &pos_ned, double heading, double ground_alt);

    void set_controls(const Controls &c) { _ctr = c; }
    void set_wind_ned(const Vec3 &w) { _wind_ned = w; }

    // advance the model by dt [s]
    void step(double dt);

    const AircraftState &state() const { return _s; }
    const AircraftParams &params() const { return _p; }
    const Controls &controls() const { return _ctr; }
    double ground_alt() const { return _ground_alt; }

private:
    AircraftParams _p;
    AircraftState _s;
    Controls _ctr;
    Vec3 _wind_ned;
    double _ground_alt{};
};

} // namespace fdm
