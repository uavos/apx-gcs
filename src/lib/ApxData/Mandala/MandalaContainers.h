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

#include <MandalaMetaTree.h>
#include <MandalaPack.h>

#include <map>
#include <variant>

namespace mandala {

using variant_t = std::variant<byte_t, word_t, dword_t, real_t>;

struct item_s
{
    const char *name;
    const char *title;
    const char *units;

    type_id_e type;
    fmt_e fmt;

    uint16_t offset;

    variant_t value;
};

constexpr auto _values_map_init = []() {
    std::map<uid_t, item_s> values_map;
    for (const auto &i : meta) {
        values_map[i.uid] = {.name = i.name,
                             .title = i.title,
                             .units = i.units,

                             .type = i.type_id,
                             .fmt = mandala::fmt(i.uid).fmt,
                             .offset = 0,
                             .value = {}};
    }
    return values_map;
};

struct values_map_t : public std::map<uid_t, item_s>
{
    values_map_t()
        : std::map<uid_t, item_s>{_values_map_init()}
    {}
};

}; // namespace mandala
