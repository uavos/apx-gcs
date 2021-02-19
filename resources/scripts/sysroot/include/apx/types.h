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

#define NULL ((void *)0)

#define CHAR_BIT 8
#define SCHAR_MIN -128
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#define CHAR_MIN 0
#define CHAR_MAX 127
#define MB_LEN_MAX 1
#define SHRT_MIN -32768
#define SHRT_MAX +32767
#define USHRT_MAX 65535
#define INT_MIN -32768
#define INT_MAX +32767
#define UINT_MAX 65535
#define LONG_MIN -2147483648
#define LONG_MAX +2147483647
#define ULONG_MAX 4294967295

#ifndef __cplusplus

#define bool _Bool
#define false 0
#define true 1

#endif /* __cplusplus */

typedef char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef long long int int64_t;

/* Unsigned.  */
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;

typedef __INTPTR_TYPE__ intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;

typedef uint32_t size_t;
typedef uint32_t uint;

/* Minimum of signed integral types.  */
#define INT8_MIN (-128)
#define INT16_MIN (-32767 - 1)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN (-__INT64_C(9223372036854775807) - 1)
/* Maximum of signed integral types.  */
#define INT8_MAX (127)
#define INT16_MAX (32767)
#define INT32_MAX (2147483647)
#define INT64_MAX (__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types.  */
#define UINT8_MAX (255)
#define UINT16_MAX (65535)
#define UINT32_MAX (4294967295U)
#define UINT64_MAX (__UINT64_C(18446744073709551615))
