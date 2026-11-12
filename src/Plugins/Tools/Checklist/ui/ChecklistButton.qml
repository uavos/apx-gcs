/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
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
import QtQuick

import Apx.Common

// Checklist button: kind selects the color, step scripts only name a kind
TextButton {
    property string kind: "ok"

    readonly property var colors: ({
        "ok": "#2e7d32",          // positive answer, confirmation
        "primary": "#1565c0",     // start an action, apply
        "danger": "#b71c1c",      // negative answer, something is wrong
        "warn": "#e65100",        // retry
        "neutral": "#37474f",     // continue, misc
    })

    size: Style.fontSize*2.4
    color: colors[kind] ? colors[kind] : colors["ok"]
    textBold: kind === "primary"
}
