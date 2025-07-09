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

#include "MandalaUid.hpp"

__BEGIN_DECLS

typedef uint8_t mandala_handle_t;
typedef uint16_t mandala_uid_t;

void mset_f(mandala_uid_t uid, float v);
void mset_i(mandala_uid_t uid, uint32_t v);

mandala_handle_t mbind(mandala_uid_t uid, const char *notify);
float mget(mandala_handle_t h);

__END_DECLS

#ifdef __cplusplus

template<mandala_uid_t UID>
class Mandala
{
public:
    explicit Mandala(const char *notify = 0) { subscribe(notify); }

    static inline constexpr mandala_uid_t uid() { return UID; }
    static inline void subscribe(const char *notify = 0) { _handle = mbind(UID, notify); }

    static inline float value() { return mget(_handle); }

    static inline void publish(float v) { mset_f(UID, v); }
    static inline void publish(uint32_t v) { mset_i(UID, v); }
    static inline void publish(bool v) { mset_i(UID, v); }

private:
    static mandala_handle_t _handle;
};
template<mandala_uid_t UID>
mandala_handle_t Mandala<UID>::_handle{255};

#endif
