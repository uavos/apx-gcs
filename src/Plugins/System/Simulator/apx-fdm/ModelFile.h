/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
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

#include "Aircraft.h"

#include <string>

namespace fdm {

// Aircraft parameters stored as a text file: one `name value` pair per line,
// `#` starts a comment. Names are the AircraftParams fields, angles are in
// degrees. Values left out keep their built-in defaults, so a file only has
// to list what differs. Vectors take several numbers:
//
//   mass        1200
//   inertia     2200 1500 3300    # Ixx Iyy Izz
//   alpha_stall 15                # deg
//   wheel       2.3 0 1.2  60e3 5e3  1 0   # x y z  k c  steerable brakes
//
// Returns false and reports the line when the file can't be read or a value
// is malformed; unknown names are reported and skipped.
bool load_model(const std::string &path, AircraftParams &params);

// Print the parameters in the same format, i.e. a template for a new model
void dump_model(const AircraftParams &params);

} // namespace fdm
