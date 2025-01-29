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
import "../common"

Column {

    readonly property var f_flaps: mandala.fact("ctr.wing.flaps")
    readonly property var f_brake: mandala.fact("ctr.str.brake")
    readonly property var f_ers: mandala.fact("ctr.ers.launch")
    readonly property var f_rel: mandala.fact("ctr.ers.rel")

    readonly property var f_eng: mandala.fact("ctr.pwr.eng")
    readonly property var f_pld: mandala.fact("ctr.pwr.payload")

    //readonly property var f_lights: mandala.fact("ctr.light.taxi")

    readonly property bool m_reg_brk: mandala.fact("cmd.reg.brk").value


    property double txtHeight
    spacing: txtHeight/8
    //anchors.fill: parent
    StatusFlag {
        id: flaps
        height: txtHeight
        visible: true
        fact: f_flaps
        text: qsTr("FLAPS")
        readonly property real v: fact.value
        show: v > 0
        status_warning: 0.6
        CleanText {
            height: txtHeight
            fact: flaps.fact
            show: flaps.show
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (flaps.v*100).toFixed()
        }
    }
    StatusFlag {
        id: brakes
        height: txtHeight
        visible: true
        fact: f_brake
        text: qsTr("BRAKE")
        readonly property real v: fact.value
        show: m_reg_brk || v > 0
        type: m_reg_brk?CleanText.Green:CleanText.Yellow
        CleanText {
            height: txtHeight
            fact: brakes.fact
            show: brakes.show && (brakes.v < 1 || m_reg_brk)
            anchors.left: parent.right
            anchors.leftMargin: 2
            text: (brakes.v*100).toFixed()
        }
    }
    Row {
        spacing: txtHeight/8
        height: txtHeight
        StatusFlag {
            id: ers_flag
            height: txtHeight
            fact: f_ers
            show: fact.value > 0
            type: CleanText.Red
            text: qsTr("ERS")
        }
        StatusFlag {
            height: txtHeight
            fact: f_rel
            show: fact.value > 0
            type: CleanText.Yellow
            text: qsTr("REL")
        }
    }
    StatusFlag {
        height: txtHeight
        visible: true
        fact: f_eng
        show: fact.value <= 0 && apx.datalink.valid
        type: CleanText.Red
        text: qsTr("ENG")
    }
    StatusFlag {
        height: txtHeight
        visible: true
        fact: f_pld
        show: fact.value > 0
        type: CleanText.Green
        text: qsTr("PYLD")
    }
    /*StatusFlag {
        height: txtHeight
        visible: true
        fact: f_lights
        show: fact.value > 0
        text: qsTr("TAXI")
    }*/
}

