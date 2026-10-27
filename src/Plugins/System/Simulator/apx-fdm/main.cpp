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

// apx-fdm: flight dynamics for SIL simulation without X-Plane.
// Runs N aircraft models in one process, each talking to its own
// `sim` SIL node instance over UDP (ports + 100*i).

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <MandalaBundles.h>
#include <tcp_ports.h>

#include "Aircraft.h"
#include "Geo.h"
#include "ModelFile.h"
#include "SimLink.h"

#ifndef VERSION
#define VERSION "dev"
#endif

using namespace fdm;

namespace {

constexpr double kPhysicsRate = 500.0; // [Hz] fixed integration step
constexpr int kPortStride = 100; // port offset per SIL instance (same as GCS Simulator plugin)
constexpr int kMaxUnits = 10;

volatile sig_atomic_t g_stop = 0;

void on_signal(int)
{
    g_stop = 1;
}

struct Options
{
    int count{0}; // 0 = auto-detect running SIL nodes
    double lat{47.0};
    double lon{8.0};
    double alt{400.0};
    double hdg{90.0};
    double spacing{50.0};
    double speedup{1.0};
    std::string host{"127.0.0.1"};
    double rate{100.0};
    double wind_spd{0.0};
    double wind_dir{0.0};
    bool verbose{false};
    std::string model;      // aircraft model file, empty = built-in defaults
    bool model_dump{false}; // print the built-in model and exit
};

void usage(const char *prog)
{
    printf("APX Autopilot project <http://docs.uavos.com>\n"
           "Flight dynamics model for SIL simulation (v" VERSION ")\n\n"
           "Usage: %s [options]\n\n"
           "Options:\n"
           "  -n, --count N       number of aircraft / SIL instances (1..%d)\n"
           "                      [auto: serve every SIL node that answers on its ports]\n"
           "      --lat DEG       start point latitude [47.0]\n"
           "      --lon DEG       start point longitude [8.0]\n"
           "      --alt M         start point (ground) altitude MSL [400]\n"
           "      --hdg DEG       start heading [90]\n"
           "      --spacing M     distance between aircraft along the heading [50]\n"
           "      --speedup X     simulation time multiplier [1.0]\n"
           "      --host ADDR     SIL node host to send sensors to [127.0.0.1]\n"
           "      --rate HZ       sensors output rate (50..100) [100]\n"
           "      --wind-spd MPS  wind speed [0]\n"
           "      --wind-dir DEG  wind direction (from) [0]\n"
           "      --model FILE    aircraft model file [built-in]\n"
           "      --model-dump    print the built-in model and exit\n"
           "  -v, --verbose       print state of every aircraft once a second\n"
           "  -h, --help          this help\n"
           "      --version       print version\n"
           "\n"
           "Aircraft #i listens for controls on UDP port %u+%d*i and sends sensors\n"
           "to host:%u+%d*i, matching `sim-darwin -x <ctr> -s <sns>` ports.\n",
           prog,
           kMaxUnits,
           UDP_PORT_SIM_CTR,
           kPortStride,
           UDP_PORT_AP_SNS,
           kPortStride);
}

bool parse_args(int argc, char **argv, Options &opt)
{
    enum {
        OPT_LAT = 1000,
        OPT_LON,
        OPT_ALT,
        OPT_HDG,
        OPT_SPACING,
        OPT_SPEEDUP,
        OPT_HOST,
        OPT_RATE,
        OPT_WIND_SPD,
        OPT_WIND_DIR,
        OPT_VERSION,
        OPT_MODEL,
        OPT_MODEL_DUMP,
    };
    static const struct option long_opts[] = {
        {"count", required_argument, nullptr, 'n'},
        {"lat", required_argument, nullptr, OPT_LAT},
        {"lon", required_argument, nullptr, OPT_LON},
        {"alt", required_argument, nullptr, OPT_ALT},
        {"hdg", required_argument, nullptr, OPT_HDG},
        {"spacing", required_argument, nullptr, OPT_SPACING},
        {"speedup", required_argument, nullptr, OPT_SPEEDUP},
        {"host", required_argument, nullptr, OPT_HOST},
        {"rate", required_argument, nullptr, OPT_RATE},
        {"wind-spd", required_argument, nullptr, OPT_WIND_SPD},
        {"wind-dir", required_argument, nullptr, OPT_WIND_DIR},
        {"verbose", no_argument, nullptr, 'v'},
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, OPT_VERSION},
        {"model", required_argument, nullptr, OPT_MODEL},
        {"model-dump", no_argument, nullptr, OPT_MODEL_DUMP},
        {nullptr, 0, nullptr, 0},
    };

    int c;
    while ((c = getopt_long(argc, argv, "n:vh", long_opts, nullptr)) != -1) {
        switch (c) {
        case 'n':
            opt.count = atoi(optarg);
            break;
        case OPT_LAT:
            opt.lat = atof(optarg);
            break;
        case OPT_LON:
            opt.lon = atof(optarg);
            break;
        case OPT_ALT:
            opt.alt = atof(optarg);
            break;
        case OPT_HDG:
            opt.hdg = atof(optarg);
            break;
        case OPT_SPACING:
            opt.spacing = atof(optarg);
            break;
        case OPT_SPEEDUP:
            opt.speedup = atof(optarg);
            break;
        case OPT_HOST:
            opt.host = optarg;
            break;
        case OPT_RATE:
            opt.rate = atof(optarg);
            break;
        case OPT_WIND_SPD:
            opt.wind_spd = atof(optarg);
            break;
        case OPT_WIND_DIR:
            opt.wind_dir = atof(optarg);
            break;
        case 'v':
            opt.verbose = true;
            break;
        case OPT_VERSION:
            printf("%s\n", VERSION);
            exit(0);
        case OPT_MODEL:
            opt.model = optarg;
            break;
        case OPT_MODEL_DUMP:
            opt.model_dump = true;
            break;
        case 'h':
        default:
            usage(argv[0]);
            exit(c == 'h' ? 0 : 1);
        }
    }

    if (opt.count < 0 || opt.count > kMaxUnits) {
        fprintf(stderr, "count must be 1..%d\n", kMaxUnits);
        return false;
    }
    if (!(opt.speedup > 0.0)) {
        fprintf(stderr, "speedup must be positive\n");
        return false;
    }
    if (opt.rate < 1.0 || opt.rate > kPhysicsRate) {
        fprintf(stderr, "rate must be 1..%.0f\n", kPhysicsRate);
        return false;
    }
    if (std::fabs(opt.lat) > 90.0 || std::fabs(opt.lon) > 180.0) {
        fprintf(stderr, "invalid start coordinates\n");
        return false;
    }
    return true;
}

struct Unit
{
    std::unique_ptr<SimLink> link;
    Aircraft aircraft;
    bool active{false}; // SIL node answered with its controls config
    Vec3 start_pos;
};

void fill_sensors(const Aircraft &ac, const GeoOrigin &geo, mandala::bundle::sim_s &sns)
{
    const AircraftState &s = ac.state();

    double roll, pitch, yaw;
    s.q.to_euler(roll, pitch, yaw);
    sns.att_deg[0] = (float) rad2deg(roll);
    sns.att_deg[1] = (float) rad2deg(pitch);
    sns.att_deg[2] = (float) rad2deg(wrap_2pi(yaw));

    for (int i = 0; i < 3; ++i) {
        sns.gyro_deg[i] = (float) rad2deg(s.omega[i]);
        sns.acc_ned[i] = (float) s.acc_ned[i];
        sns.vel_ned[i] = (float) s.vel_ned[i];
    }

    double lat, lon, hmsl;
    geo.to_llh(s.pos_ned, lat, lon, hmsl);
    sns.lat_deg = lat;
    sns.lon_deg = lon;
    sns.hmsl = (float) hmsl;

    sns.agl = (float) (hmsl - ac.ground_alt());
    sns.rpm = (float) s.rpm;

    sns.airspeed = (float) s.eas;
    sns.baro_mbar = (float) (s.pressure / 100.0);

    sns.air_temp = (float) s.air_temp;
    sns.room_temp = (float) (s.air_temp + 20.0);

    sns.slip = (float) rad2deg(s.beta);
}

void print_status(const Unit &u, const GeoOrigin &geo, double sim_t)
{
    const AircraftState &s = u.aircraft.state();
    const Controls &c = u.aircraft.controls();

    double roll, pitch, yaw;
    s.q.to_euler(roll, pitch, yaw);
    double lat, lon, hmsl;
    geo.to_llh(s.pos_ned, lat, lon, hmsl);

    printf("[sim:%d] t=%6.1f %s %s lat=%.6f lon=%.6f hmsl=%6.1f agl=%6.1f tas=%5.1f gs=%5.1f "
           "hdg=%5.1f roll=%5.1f pitch=%5.1f vs=%5.1f aoa=%4.1f thr=%.2f rpm=%4.0f "
           "ail=%5.2f elv=%5.2f rud=%5.2f flp=%.2f brk=%.2f\n",
           u.link->index(),
           sim_t,
           u.link->configured() ? "CTR" : "---",
           s.on_ground ? "GND" : "AIR",
           lat,
           lon,
           hmsl,
           hmsl - u.aircraft.ground_alt(),
           s.tas,
           std::hypot(s.vel_ned.x, s.vel_ned.y),
           rad2deg(wrap_2pi(yaw)),
           rad2deg(roll),
           rad2deg(pitch),
           -s.vel_ned.z,
           rad2deg(s.alpha),
           c.thr,
           s.rpm,
           c.ail,
           c.elv,
           c.rud + c.rud_trim,
           c.flaps,
           c.brake);
    fflush(stdout);
}

} // namespace

int main(int argc, char **argv)
{
    Options opt;
    if (!parse_args(argc, argv, opt))
        return 1;

    AircraftParams params;
    if (!opt.model.empty() && !fdm::load_model(opt.model, params))
        return 1;

    if (opt.model_dump) {
        // the built-in model, or the given one with its values applied
        fdm::dump_model(params);
        return 0;
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    const int n_units = opt.count > 0 ? opt.count : kMaxUnits;

    printf("APX flight dynamics model v" VERSION "\n");
    printf("start: lat=%.6f lon=%.6f alt=%.1f hdg=%.1f, spacing %.0f m, speedup x%.2f\n",
           opt.lat,
           opt.lon,
           opt.alt,
           opt.hdg,
           opt.spacing,
           opt.speedup);
    if (opt.count > 0)
        printf("%d aircraft\n", opt.count);
    else
        printf("auto-detect: up to %d SIL nodes, controls ports %u..%u step %d\n",
               kMaxUnits,
               UDP_PORT_SIM_CTR,
               UDP_PORT_SIM_CTR + kPortStride * (kMaxUnits - 1),
               kPortStride);
    if (!opt.model.empty())
        printf("model: %s\n", opt.model.c_str());
    if (opt.wind_spd > 0)
        printf("wind: %.1f m/s from %.0f deg\n", opt.wind_spd, opt.wind_dir);
    fflush(stdout);

    GeoOrigin geo;
    geo.init(opt.lat, opt.lon, opt.alt);

    const double hdg = deg2rad(opt.hdg);
    const Vec3 wind_ned = Vec3(std::cos(deg2rad(opt.wind_dir)), std::sin(deg2rad(opt.wind_dir)), 0.0)
                          * (-opt.wind_spd);

    std::vector<Unit> units(n_units);
    for (int i = 0; i < n_units; ++i) {
        Unit &u = units[i];
        u.link = std::make_unique<SimLink>(i,
                                           opt.host,
                                           (uint16_t) (UDP_PORT_SIM_CTR + kPortStride * i),
                                           (uint16_t) (UDP_PORT_AP_SNS + kPortStride * i));
        u.link->open();

        u.start_pos = Vec3(std::cos(hdg) * opt.spacing * i, std::sin(hdg) * opt.spacing * i, 0.0);
        u.aircraft = Aircraft(params);
        u.aircraft.set_wind_ned(wind_ned);
        u.aircraft.init(u.start_pos, hdg, opt.alt);
    }

    using clock = std::chrono::steady_clock;
    const auto t_start = clock::now();
    auto wall = [&]() { return std::chrono::duration<double>(clock::now() - t_start).count(); };

    const double dt = 1.0 / kPhysicsRate;
    const int sns_div = std::max(1, (int) std::lround(kPhysicsRate / opt.rate));

    double sim_t = 0.0;
    uint64_t steps = 0;
    double t_tick = -1.0;
    double t_status = 0.0;
    double t_behind_warn = -1e9;
    double t_waiting = -1e9;

    mandala::bundle::sim_s sns{};

    while (!g_stop) {
        const double now = wall();

        for (auto &u : units) {
            u.link->poll(now);

            // activate the model when its SIL node shows up, park it when the node goes away
            const bool configured = u.link->configured();
            if (configured && !u.active) {
                u.active = true;
                printf("[sim:%d] SIL node active\n", u.link->index());
                fflush(stdout);
            } else if (!configured && u.active) {
                u.active = false;
                if (u.aircraft.state().on_ground) {
                    // back to the start point for the next node run
                    u.aircraft.init(u.start_pos, hdg, opt.alt);
                    printf("[sim:%d] SIL node gone, aircraft reset to start\n", u.link->index());
                } else {
                    printf("[sim:%d] SIL node gone in flight, aircraft keeps flying\n",
                           u.link->index());
                }
                fflush(stdout);
            }
        }

        const double target = now * opt.speedup;
        if (target - sim_t > 1.0) {
            // can't keep up with real time - skip ahead instead of spiralling
            if (now - t_behind_warn > 5.0) {
                t_behind_warn = now;
                printf("warning: simulation is %.1f s behind real time, resyncing\n",
                       target - sim_t);
                fflush(stdout);
            }
            sim_t = target - dt;
        }

        while (sim_t + dt <= target && !g_stop) {
            for (auto &u : units) {
                if (!u.active)
                    continue;
                u.aircraft.set_controls(u.link->controls());
                u.aircraft.step(dt);
            }
            sim_t += dt;
            steps++;

            if (steps % sns_div == 0) {
                for (auto &u : units) {
                    if (!u.active)
                        continue;
                    fill_sensors(u.aircraft, geo, sns);
                    u.link->send_sensors(sns);
                }
            }
        }

        if (now - t_tick >= 0.1) {
            t_tick = now;
            for (auto &u : units)
                u.link->tick(now);
        }

        if (opt.verbose && sim_t - t_status >= 1.0) {
            t_status = sim_t;
            int n_active = 0;
            for (const auto &u : units) {
                if (!u.active)
                    continue;
                n_active++;
                print_status(u, geo, sim_t);
            }
            if (!n_active && now - t_waiting >= 5.0) {
                t_waiting = now;
                printf("waiting for SIL nodes...\n");
                fflush(stdout);
            }
        }

        const double wait = (sim_t + dt) / opt.speedup - wall();
        if (wait > 0)
            std::this_thread::sleep_for(std::chrono::duration<double>(std::min(wait, 0.002)));
    }

    printf("\napx-fdm terminated after %.1f s of simulation (%llu steps)\n",
           sim_t,
           (unsigned long long) steps);
    for (const auto &u : units) {
        if (!u.link->rx_ctr_count() && !u.link->tx_sns_count())
            continue;
        printf("[sim:%d] rx controls: %llu, tx sensors: %llu\n",
               u.link->index(),
               (unsigned long long) u.link->rx_ctr_count(),
               (unsigned long long) u.link->tx_sns_count());
    }
    fflush(stdout);
    return 0;
}
