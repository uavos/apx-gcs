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

using alt = Mandala<mandala::cmd::nav::pos::altitude>;

using roll = Mandala<mandala::est::nav::att::roll>;
using u1 = Mandala<mandala::est::env::usr::u1>;

int main()
{
    alt(); // subscribe
    u1();

    task("on_task", 1000);

    return 0;
}

EXPORT void on_task()
{
    alt::publish(alt::value() + 1.f);
    u1::publish(u1::value() + 0.1f);

    const uint8_t tbuf[]{1, 2, 3, 4, 5, 6, 7, 8, 9};
    send(1, tbuf, sizeof(tbuf), true);
}
