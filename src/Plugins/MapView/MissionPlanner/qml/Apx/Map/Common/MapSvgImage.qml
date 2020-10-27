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
import QtQuick 2.2
import QtGraphicalEffects 1.0

import Apx.Common 1.0

SvgImage {
    id: control
    property bool glow: true
    property color glowColor: "#000"

    effect: glow?glowC:blurC

    Component {
        id: glowC
        Glow {
            id: imageGlow
            anchors.fill: control
            radius: ui.effects?4:0
            samples: 8
            color: glowColor
        }
    }
    Component {
        id: blurC
        FastBlur {
            id: imageBlur
            anchors.fill: control
            radius: ui.antialiasing?Math.max(width,height)/10:0
        }
    }
}
