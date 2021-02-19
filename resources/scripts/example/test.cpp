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

struct test_s
{
    float v;
};

template<typename T>
struct tmpl_s
{
    T value;
};

class Test
{
public:
    explicit Test()
    {
    }
};

int main()
{
    test_s t_f;
    tmpl_s<uint32_t> t_u;

    Test test;

    //memcpy(&t_f, &test, sizeof(t));

    t_f.v = (1.2);
    t_u.value = 1234;

    log("Hello world!");

    for (unsigned int i = 1; i > 0; ++i) {
        t_f.v += 0.1f;
    }
    log("u: %u", t_u.value);
    log("f: %f", t_f.v);

    int32_t si = -1234;
    log("i: %f", si);

    return 0;
}

EXPORT void hello()
{
    log("TEST!");
}
