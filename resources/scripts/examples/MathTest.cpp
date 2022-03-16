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

#define E 2.718281828f

using roll = Mandala<mandala::est::nav::att::roll>;

EXPORT void testMath();

EXPORT bool testAtan2(float y, float x);
EXPORT bool testLog(float y);
EXPORT bool testSqrt(float y);

uint32_t start_time;

int main()
{
    puts("Start Test Math...\n");

    roll();

    testMath();

    start_time = time_ms();

    task("on_task", 100);

    return 0;
}

void testTrigonometric(float x)
{
    float val = x;
    float deg_val = val * R2D;

    float sin_val = sin(val);
    float cos_val = cos(val);
    float tan_val = tan(val);

    float a_sin_val = asin(sin_val);
    float a_cos_val = acos(cos_val);
    float a_tan_val = atan(tan_val);

    if (fabs(sin(a_sin_val) - sin_val) > 0.0001f) {
        puts("Fail sin()...\n");
        printf("val deg=%.2f\n", deg_val);
        printf("sin=%.4f\n", sin_val);
        printf("asin=%.4f\n", sin(a_sin_val));
        puts("----------\n");
    }

    if (fabs(cos(a_cos_val) - cos_val) > 0.0001f) {
        puts("Fail cos()...\n");
        printf("val deg=%.2f\n", deg_val);
        printf("cos=%.4f\n", cos_val);
        printf("acos=%.4f\n", cos(a_cos_val));
        puts("----------\n");
    }

    if (fabs(tan(a_tan_val) - tan_val) > 0.001f) {
        puts("Fail tan()...\n");
        printf("val deg=%.2f\n", deg_val);
        printf("tan=%.4f\n", tan_val);
        printf("atan=%.4f\n", tan(a_tan_val));
        puts("----------\n");
    }
}

void testMath()
{
    print("-----test exp-----\n");
    printf("exp(-1)=%.5f\n", exp(-1.0)); // 0.367879441171442
    printf("exp(0)=%.1f\n", exp(0));     // 1
    printf("exp(1)=%.5f\n", exp(1.0));   // 2.718281828459045

    print("-----test atat2-----\n");
    float y = -1.0;
    bool ok = true;
    do {
        if (!testAtan2(y, 1.0))
            ok = false;
        y += 0.01f;
    } while (y <= 1.f);
    printf("Test atat2: %s\n", ok ? "Ok" : "Fail");

    print("-----test log-----\n");
    y = 1.0;
    ok = true;
    do {
        if (!testLog(y))
            ok = false;
        y += 1.f;
    } while (y <= 1000.f);
    printf("Test log: %s\n", ok ? "Ok" : "Fail");

    print("-----test sqrt-----\n");
    y = 0.0;
    ok = true;
    do {
        if (!testSqrt(y))
            ok = false;
        y += 1.f;
    } while (y <= 10000.f);
    printf("Test sqrt: %s\n", ok ? "Ok" : "Fail");
}

bool testAtan2(float y, float x)
{
    bool ok = fabs(atan2(y, x) - atan(y / x)) > 0.0001f;
    if (ok) {
        printf("Fail atan2 %f\n", atan2(y, x));
    }
    return !ok;
}

bool testLog(float y)
{
    float val = log(y);
    bool ok = fabs(y - pow(E, val)) > 0.001f;
    if (ok) {
        print("Fail log()...\n");
        printf("y=%.3f\n", y);
        printf("ln(y)=%.3f\n", val);
        printf("pow(e,ln(y))=%.3f\n", pow(E, val));
    }
    return !ok;
}

bool testSqrt(float y)
{
    float val = sqrt(y);
    bool ok = fabs(y - pow(val, 2)) > 0.001f;
    if (ok) {
        print("Fail sqrt()...\n");
        printf("x=%.3f\n", y);
        printf("sqrt(y)=%.3f\n", val);
        printf("pow(sqrt(y),2)=%.3f\n", pow(val, 2));
    }
    return !ok;
}

EXPORT void on_task()
{
    float val = roll::value();
    testTrigonometric(val);

    val = (float) (time_ms() / start_time) - val * 1000.f;

    testAtan2(val, 1.0);
    testLog(val);
    testSqrt(val);
}
