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
    explicit Test() { f = 5.4321f; }
    double d{1.234567};
    float f;
};

char tbuf[256];

using alt = Mandala<mandala::cmd::nav::pos::altitude>;

using roll = Mandala<mandala::est::nav::att::roll>;
using u1 = Mandala<mandala::est::env::usr::u1>;

static constexpr const port_id_t port_id{1};

static uint32_t count{0};

int main()
{
    puts("Hello world!");
    alt("on_alt"); // subscribe

    roll();

    test_s t_f;
    tmpl_s<uint32_t> t_u;

    Test test;

    t_f.v = 1.2f;
    t_u.value = 1234;

    float vf = 1.234f;
    printf("vf: %.2f", vf);
    for (uint i = 1; i < 1110; ++i) {
        t_f.v += 0.1f;
        vf += 1.0002;
    }
    printf("VALUES:\n"); // newline is unnecessary
    printf("u: %u", t_u.value);
    printf("x: %.4X", t_u.value);

    vf += 1.23f;
    printf("f: %.2f", vf);
    printf("d: %f", (float) test.d);
    printf("f: %f", test.f);

    int32_t si = -1234;
    printf("i: %i", si);
    printf("iu: %u", si);

    const char *s = "test string";
    printf("s: %s", s);

    memset(tbuf, 0, sizeof(tbuf));
    snprintf(tbuf, sizeof(tbuf), "(%.3f)", vf + 44);
    printf("s2: %s", tbuf);

    for (auto i = 0; i < 2; ++i) {
        sleep(200);
        printf("sleep time:%u", time_ms());
    }

    // set and publish mandala value
    Mandala<mandala::cmd::nav::pos::airspeed>::publish(vf);
    alt::publish(t_u.value);

    for (auto i = 0; i < 3; ++i) {
        sleep(100);
        printf("%.2f", alt::value());
    }

    // task("on_task", 100);

    receive(port_id, "on_serial");

    task("on_roll_task", 1);

    return 0;
}

EXPORT void on_alt()
{
    printf("on_alt: %.2f", alt::value());

    // printf("tx:%u",count++);
    const uint8_t tbuf[]{1, 2, 3, 4, 5, 6, 7, 8, 9};
    send(port_id, tbuf, sizeof(tbuf), true);
    // sleep(1000);  // simulate events queue
}

uint8_t cnt = 5;

EXPORT void on_task()
{
    // printf("on_task: %u", time_ms());
    alt::publish(alt::value() + 1);
    if (!cnt--)
        exit();
}

EXPORT void on_serial(const uint8_t *data, size_t size)
{
    (void) data;
    printf("rx: %u", size);
}

EXPORT void on_roll_task()
{
    u1::publish(roll::value() * (180.f / 3.14f));
}
