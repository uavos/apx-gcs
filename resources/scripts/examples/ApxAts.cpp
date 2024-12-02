/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Shared Libraries.
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

#include <apx.h>

using m_f1 = Mandala<mandala::est::env::usrf::f1>; //unit lat
using m_f2 = Mandala<mandala::est::env::usrf::f2>; //unit lon
using m_f3 = Mandala<mandala::est::env::usrf::f3>; //unit hmsl

using m_ats_mode = Mandala<mandala::cmd::nav::ats::mode>;

static constexpr const port_id_t port_id{5};

float unit_pos[3] = {};
bool new_data = {};

int main()
{
    task("on_unit_pos", 20); // 50 Hz
    receive(port_id, "on_ats");

    m_f1();
    m_f2();
    m_f3();

    m_ats_mode();

    return 0;
}

EXPORT void on_unit_pos()
{
    if (!new_data) {
        return;
    }

    m_f1::publish(unit_pos[0]);
    m_f2::publish(unit_pos[1]);
    m_f3::publish(unit_pos[2]);

    new_data = false;
}

EXPORT void on_ats(const uint8_t *data, size_t size)
{
    if (size != sizeof(unit_pos)) {
        return;
    }

    memcpy(unit_pos, data, size);
    new_data = true;
}
