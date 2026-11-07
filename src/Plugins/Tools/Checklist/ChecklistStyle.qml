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
pragma Singleton
import QtQuick

import Apx.Common

/*
    Look of the checklist window: colors, sizes and button kinds.
    Markdown scripts only name a kind (ok, primary, danger, neutral), the colors live here.
*/
QtObject {
    readonly property real fs: Style.fontSize
    readonly property real sp: Style.spacing

    // window
    readonly property color background: "#101010"
    readonly property color separator: "#444"
    readonly property color panel: "#1a1a1a"

    // text
    readonly property color text: "#ddd"
    readonly property color textBright: "#fff"
    readonly property color textMuted: "#777"
    readonly property color textInfo: "#ffbe00"        // live message of the running step
    readonly property real fontTitle: fs*2
    readonly property real fontStep: fs*1.8
    readonly property real fontBody: fs*1.5
    readonly property real fontList: fs*1.3
    readonly property real fontSmall: fs*1.3

    // step status (list dots and result banner)
    readonly property color ok: "#4cab50"
    readonly property color fail: "#ff5050"
    readonly property color running: "#fafe86"
    readonly property color pending: "#555"
    readonly property color okBg: "#2040ff40"
    readonly property color failBg: "#40ff0000"

    // steps list
    readonly property real rowHeight: fs*2.6
    readonly property color currentBg: "#2b3d5c"       // step being executed
    readonly property color viewedBg: "#30ffffff"      // step the operator is reading
    readonly property color currentMark: "#4a8df8"     // bar at the left of the current step

    // buttons by kind
    readonly property real buttonSize: fs*2.4
    readonly property var buttonColors: ({
        "ok": "#2e7d32",          // positive answer, confirmation
        "primary": "#1565c0",     // start an action, apply
        "danger": "#b71c1c",      // negative answer, something is wrong
        "warn": "#e65100",        // retry
        "neutral": "#37474f",     // continue, misc
    })
    function buttonColor(kind) { return buttonColors[kind] ? buttonColors[kind] : buttonColors["ok"] }

    // input fields
    readonly property real fieldWidth: fs*8
    readonly property real fontField: fs*1.4
}
