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
#include "ModelFile.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace fdm {

namespace {

struct Field
{
    const char *name;
    double *value;
    bool degrees; // stored in radians, written and read as degrees
};

// every scalar of AircraftParams, in the order of its declaration
static std::vector<Field> fields_of(AircraftParams &p)
{
    return {
        {"mass", &p.mass, false},

        {"S", &p.S, false},
        {"b", &p.b, false},
        {"c", &p.c, false},

        {"CL0", &p.CL0, false},
        {"CLa", &p.CLa, false},
        {"CLq", &p.CLq, false},
        {"CLde", &p.CLde, false},
        {"CLdf", &p.CLdf, false},
        {"alpha_stall", &p.alpha_stall, true},
        {"stall_width", &p.stall_width, true},

        {"CD0", &p.CD0, false},
        {"e", &p.e, false},
        {"CDdf", &p.CDdf, false},
        {"CDsb", &p.CDsb, false},
        {"CDb", &p.CDb, false},

        {"CYb", &p.CYb, false},
        {"CYp", &p.CYp, false},
        {"CYr", &p.CYr, false},
        {"CYdr", &p.CYdr, false},

        {"Clb", &p.Clb, false},
        {"Clp", &p.Clp, false},
        {"Clr", &p.Clr, false},
        {"Clda", &p.Clda, false},
        {"Cldr", &p.Cldr, false},

        {"Cm0", &p.Cm0, false},
        {"Cma", &p.Cma, false},
        {"Cmq", &p.Cmq, false},
        {"Cmde", &p.Cmde, false},
        {"Cmdf", &p.Cmdf, false},

        {"Cnb", &p.Cnb, false},
        {"Cnp", &p.Cnp, false},
        {"Cnr", &p.Cnr, false},
        {"Cndr", &p.Cndr, false},
        {"Cnda", &p.Cnda, false},

        {"thrust_static", &p.thrust_static, false},
        {"power", &p.power, false},
        {"rpm_idle", &p.rpm_idle, false},
        {"rpm_max", &p.rpm_max, false},
        {"engine_tau", &p.engine_tau, false},

        {"steer_max", &p.steer_max, true},
        {"steer_fade_speed", &p.steer_fade_speed, false},
        {"mu_roll", &p.mu_roll, false},
        {"mu_brake", &p.mu_brake, false},
        {"mu_side", &p.mu_side, false},
        {"slip_speed", &p.slip_speed, false},
    };
}

constexpr size_t wheels_cnt = sizeof(AircraftParams::wheels) / sizeof(Wheel);

} // namespace

bool load_model(const std::string &path, AircraftParams &params)
{
    std::ifstream f(path);
    if (!f) {
        printf("model: can't read %s\n", path.c_str());
        return false;
    }

    auto fields = fields_of(params);
    size_t wheel_idx = 0;
    std::string line;

    for (int lineno = 1; std::getline(f, line); ++lineno) {
        const auto comment = line.find('#');
        if (comment != std::string::npos)
            line.erase(comment);

        std::istringstream is(line);
        std::string name;
        if (!(is >> name))
            continue; // blank line

        auto number = [&](double &v) {
            if (is >> v)
                return true;
            printf("model: %s:%d: bad value of `%s`\n", path.c_str(), lineno, name.c_str());
            return false;
        };

        if (name == "inertia") {
            double x, y, z;
            if (!number(x) || !number(y) || !number(z))
                return false;
            params.inertia = Vec3(x, y, z);
            continue;
        }

        if (name == "wheel") {
            if (wheel_idx >= wheels_cnt) {
                printf("model: %s:%d: only %zu wheels are supported\n",
                       path.c_str(),
                       lineno,
                       wheels_cnt);
                return false;
            }
            double x, y, z, k, c, steerable, brakes;
            if (!number(x) || !number(y) || !number(z) || !number(k) || !number(c)
                || !number(steerable) || !number(brakes))
                return false;
            params.wheels[wheel_idx++] = Wheel{Vec3(x, y, z), k, c, steerable != 0.0, brakes != 0.0};
            continue;
        }

        bool found = false;
        for (const auto &i : fields) {
            if (name != i.name)
                continue;
            double v;
            if (!number(v))
                return false;
            *i.value = i.degrees ? deg2rad(v) : v;
            found = true;
            break;
        }

        if (!found)
            printf("model: %s:%d: unknown parameter `%s`\n", path.c_str(), lineno, name.c_str());
    }

    return true;
}

void dump_model(const AircraftParams &params)
{
    // the table needs a mutable instance, the values are only read
    AircraftParams p = params;

    printf("# apx-fdm aircraft model\n");
    printf("# angles are in degrees, everything else in SI units\n\n");

    for (const auto &i : fields_of(p)) {
        printf("%-18s %g\n", i.name, i.degrees ? rad2deg(*i.value) : *i.value);
        if (std::string(i.name) == "mass")
            printf("%-18s %g %g %g\n", "inertia", p.inertia.x, p.inertia.y, p.inertia.z);
    }

    printf("\n# x y z  k c  steerable brakes\n");
    for (size_t i = 0; i < wheels_cnt; ++i) {
        const auto &w = p.wheels[i];
        printf("%-18s %g %g %g  %g %g  %d %d\n",
               "wheel",
               w.pos.x,
               w.pos.y,
               w.pos.z,
               w.k,
               w.c,
               w.steerable ? 1 : 0,
               w.brakes ? 1 : 0);
    }
}

} // namespace fdm
