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
import QtQuick.Layouts

import Apx.Common

// fuel pumps control page (lite profile)
Page {

    readonly property var f_pump1: mandala.ctr.sw.sw5
    readonly property var f_pump2: mandala.ctr.sw.sw6
    readonly property var f_pump3: mandala.ctr.sw.sw7
    readonly property var f_manual: mandala.ctr.sw.sw8

    component SwButton: CtrButton {
        Layout.fillWidth: true
        highlighted: fact.value > 0
        onTriggered: fact.value = highlighted ? 0 : 1
    }

    GridLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        width: parent.width
        columns: 2
        rowSpacing: buttonSpacing*2
        columnSpacing: buttonSpacing*2

        SwButton { text: qsTr("PUMP 1"); fact: f_pump1 }
        SwButton { text: qsTr("PUMP 2"); fact: f_pump2 }
        SwButton { text: qsTr("PUMP 3"); fact: f_pump3 }
        SwButton { text: qsTr("MANUAL"); fact: f_manual; info: qsTr("Manual fuel pumps mode") }
    }
}
