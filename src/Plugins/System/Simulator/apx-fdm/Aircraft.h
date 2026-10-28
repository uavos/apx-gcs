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

#include <vector>

#include "Vec3.h"

namespace fdm {

// Control inputs of the model, all in the units the SIL node sends
// for the corresponding X-Plane datarefs:
//  - surfaces -1..1 (positive: roll right, nose up, nose right)
//  - throttle, flaps, brake, airbrake 0..1
struct Controls
{
    double ail{};        // sim/joystick/yoke_roll_ratio (cyclic roll for a helicopter)
    double elv{};        // sim/joystick/yoke_pitch_ratio (cyclic pitch for a helicopter)
    double rud{};        // sim/joystick/yoke_heading_ratio (rudder + nose wheel steering),
                         // sim/flightmodel/engine/ENGN_thro_use[1] (tail rotor of a helicopter)
    double rud_trim{};   // sim/flightmodel/controls/rud_trim (rudder only)
    double thr{};        // sim/flightmodel/engine/ENGN_thro_use[0]
    double flaps{};      // sim/flightmodel/controls/flaprqst
    double brake{};      // sim/flightmodel/controls/parkbrake
    double airbrake{};   // sim/flightmodel/controls/sbrkrqst
    double collective{}; // sim/flightmodel/engine/POINT_pitch_deg[0]: rotor blade pitch [deg]
    double launch{};     // uavos/launch: catapult release (0/1)
    double parachute{};  // uavos/parachute: parachute deploy (0/1)
};

struct Wheel
{
    Vec3 pos;       // body frame position from CG [m] (x fwd, y right, z down)
    double k;       // spring stiffness [N/m]
    double c;       // damping [N*s/m]
    bool steerable; // follows the rudder/steering command
    bool brakes;    // affected by the brake command
};

enum class AircraftType { FixedWing, Helicopter };

// Rigid body 6-DOF aircraft. A fixed wing uses linear aerodynamic
// coefficients (stability and control derivatives), a power-limited
// propeller and three-wheel landing gear, optionally a catapult launcher
// and a parachute. A helicopter uses a simple main rotor (collective,
// cyclic tilt, inflow), a tail rotor and skids.
// Default values describe a ~1200 kg MALE class UAV, roughly the S1C
// model used with X-Plane: stalls at ~29 m/s clean, cruises at ~40 m/s.
struct AircraftParams
{
    AircraftType type{AircraftType::FixedWing};

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

    // propulsion: T = thr * power / (V + power/thrust_static), or with a
    // fixed pitch propeller T = thr * thrust_static * (1 - V/prop_pitch_speed)
    double thrust_static{5500.0}; // [N]
    double power{150000.0};       // propeller shaft power * efficiency [W]
    double prop_pitch_speed{
        0.0}; // [m/s] airspeed where a fixed pitch propeller stops pulling, 0: power model
    double rpm_idle{1500.0};
    double rpm_max{6500.0};
    double engine_tau{0.4}; // engine speed time constant [s]

    // catapult launcher (fixed wing): the aircraft is held on the rail
    // until the launch command, then pushed along the body x axis
    bool launcher{false};
    double launch_acc{30.0};         // [m/s^2]
    double launch_time{0.7};         // [s]
    double rail_pitch{deg2rad(8.0)}; // rail angle
    double rail_height{1.0};         // CG above ground on the rail [m]

    // parachute recovery (fixed wing): canopy drag at an attachment
    // point above the CG, deployed once and for the rest of the flight
    bool parachute{false};
    double chute_cda{15.0};            // Cd*A of the canopy [m^2]
    Vec3 chute_attach{0.0, 0.0, -1.5}; // body frame [m]
    double chute_damping{2.0};         // extra angular damping [N*m*s]

    // helicopter: thrust T = kT * (rpm/rpm_nominal)^2 * (collective + k_inflow*w - theta0)
    struct Rotor
    {
        double hub_height{0.5}; // main rotor hub above CG [m]
        double kT{37.7};        // thrust per degree of effective collective at nominal rpm [N/deg]
        double theta0{-2.0};    // zero thrust collective [deg]
        double k_inflow{0.4};   // collective equivalent of vertical airspeed [deg per m/s]
        double k_tl{
            0.015}; // translational lift: thrust gain per m/s of horizontal airspeed (max +30%)
        double tilt_max{deg2rad(15.0)}; // thrust vector tilt at full cyclic
        double k_hub{10.0};             // hub spring moment per unit cyclic at nominal rpm [N*m]
        Vec3 damping{4.0, 6.0, 3.0};    // angular damping [N*m*s]
        double tail_arm{1.3};           // tail rotor arm behind CG [m]
        double k_tail{40.0};            // tail rotor thrust at full command [N]
        double k_torque{0.08};          // main rotor reaction torque per N of thrust [m]
        double cda{0.15};               // fuselage drag area [m^2]
        double k_fin{
            0.09}; // vertical fin at tail_arm: side force per (sideslip speed * airspeed) [N/(m/s)^2]
        double rpm_nominal{8000.0}; // governor set point used for rotor speed scaling
        double rpm_free{10000.0};   // engine rpm at full throttle without load
        double rpm_droop{150.0};    // rpm lost per degree of collective load at full throttle
        double rpm_tau{1.5};        // engine speed time constant [s]
    } rotor;

    // ground handling
    double steer_max{deg2rad(20.0)}; // nose wheel deflection at full command
    double steer_fade_speed{20.0};   // steering authority halves at this ground speed [m/s]
    double mu_roll{0.03};            // rolling resistance
    double mu_brake{0.6};            // additional friction at full brake
    double mu_side{0.9};             // lateral tyre friction
    double slip_speed{0.3};          // friction saturates above this slip speed [m/s]

    // any number of contact points: wheels or skids
    std::vector<Wheel> wheels{
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
    double agl{};      // CG above the ground [m]
    double tas{};      // true airspeed [m/s]
    double eas{};      // equivalent airspeed [m/s]
    double alpha{};    // angle of attack [rad]
    double beta{};     // sideslip [rad]
    double rho{};      // air density [kg/m^3]
    double pressure{}; // static pressure [Pa]
    double air_temp{}; // [C]
    bool on_ground{true};

    // launcher / parachute
    bool on_rail{false};
    bool launched{false};
    double t_launch{};
    bool chute{false};
};

class Aircraft
{
public:
    explicit Aircraft(const AircraftParams &params = {});

    void set_params(const AircraftParams &params) { _p = params; }

    // Place the aircraft on the ground (or on the launcher rail) at NED
    // position (z is ground level) with the given heading [rad].
    // ground_alt is the MSL altitude of the (flat) ground.
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
    void fixed_wing_forces(const Vec3 &v_air_b, double V, double rho, double dt, Vec3 &F, Vec3 &M);
    void helicopter_forces(const Vec3 &v_air_b, double V, double rho, double dt, Vec3 &F, Vec3 &M);
    void gear_forces(const Vec3 &v_gnd_b, Vec3 &F, Vec3 &M, bool &on_ground);

    AircraftParams _p;
    AircraftState _s;
    Controls _ctr;
    Vec3 _wind_ned;
    double _ground_alt{};
    double _t{}; // model time [s]
};

} // namespace fdm
