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

#include "visibility.h"
#include "types.h"

__BEGIN_DECLS

#define R2D  57.29577951308232          // radians to degrees conversion factor
#define D2R   0.01745329251994          // degrees to radians conversion factor
#define PI    3.14159265358979          // PI
#define PI_2  1.57079632679489          // PI/2
#define PI_4  0.78539816339744          // PI/4

int abs(int x);
float acos(float x);
float asin(float x);
float atan(float x);
float atan2(float y, float x);
float ceil(float x);
float cos(float x);
float exp(float x);
float fabs(float x);
float floor(float x);
float fmod(float x, float y);
float log(float x);
float log10(float x);
float pow(float x, float y);
float sin(float x);
float sqrt(float x);
float tan(float x);

__END_DECLS
