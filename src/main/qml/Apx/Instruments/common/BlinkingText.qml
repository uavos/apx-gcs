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
import "."

CleanText {
    id: control

    property bool blinking: false
    property bool blinkingFG: true

    property int interval: 300

    readonly property bool hide_blink: !(ui.test || (show && blink))

    hide_bg: hide_blink
    hide_fg: blinkingFG ? hide_blink : !(show || ui.test)

    property bool blink: true
    onBlinkingChanged: blink=true

    SequentialAnimation on visible {
        running: blinking
        loops: Animation.Infinite
        PropertyAction { target: control; property: "blink"; value: true }
        PauseAnimation { duration: control.interval }
        PropertyAction { target: control; property: "blink"; value: false }
        PauseAnimation { duration: control.interval }
    }

}

