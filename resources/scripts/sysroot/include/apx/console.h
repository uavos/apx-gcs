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
#pragma once

#include "types.h"
#include "visibility.h"

__BEGIN_DECLS

int puts(const char *v);
void printf_s(const char *fmt, const char *v);
void printf_f(const char *fmt, float v);
void printf_i(const char *fmt, int32_t v);

__END_DECLS

#ifdef __cplusplus

void print(const char *v)
{
    puts(v);
}
void printf(const char *v)
{
    print(v);
}
void printf(const char *fmt, const char *v)
{
    printf_s(fmt, v);
}
void printf(const char *fmt, float v)
{
    printf_f(fmt, v);
}
void printf(const char *fmt, int32_t v)
{
    printf_i(fmt, v);
}
void printf(const char *fmt, uint32_t v)
{
    printf_i(fmt, v);
}

#endif
