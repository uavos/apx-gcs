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
import QtQuick.Controls.Material
import QtQml.Models

import Apx.Common

RowLayout {

    readonly property var f_mode: mandala.fact("cmd.proc.mode")
    readonly property var f_stage: mandala.fact("cmd.proc.stage")
    readonly property var f_action: mandala.fact("cmd.proc.action")
    readonly property var f_adj: mandala.fact("cmd.proc.adj")

    readonly property var f_flaps: mandala.fact("ctr.wing.flaps")
    readonly property var f_airbrk: mandala.fact("ctr.wing.airbrk")
    readonly property var f_brake: mandala.fact("ctr.str.brake")
    readonly property var f_thr: mandala.fact("cmd.rc.thr")

    readonly property bool m_reg_taxi: mandala.fact("cmd.reg.taxi").value

    spacing: buttonSpacing*4

    ColumnLayout {
        Layout.alignment: Qt.AlignTop|Qt.AlignLeft
        spacing: buttonSpacing
        CtrNum { title: "THR"; fact: f_thr; min: 0; max: 100; mult: 100; stepSize: 1; }
        CtrNum { title: "FLP"; fact: f_flaps; min: 0; max: 100; mult: 100; stepSize: 10; }
        CtrNum { title: "ADJ"; fact: f_adj; }
        CtrNum { title: "ABR"; fact: f_airbrk; min: 0; max: 100; mult: 100; stepSize: 10; visible: f_mode.value == f_mode.eval.LANDING}
    }
    CtrFlow {
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        key: f_mode.text
        controls: {
            "TAXI": [btnCANCEL,btnATAXI,btnINC,btnDEC,btnBRK_TAXI],
            "TAKEOFF": [btnCANCEL,btnNEXT,btnBRK],
            "LANDING": [btnCANCEL,btnNEXT,btnBRK,btnAIRBRK],
            "WPT": [btnINC,btnDEC],
            "STBY": [btnRESET],
            "UAV": [btnNEXT,btnCANCEL],
        }
    }


    Component {
        id: btnFLP
        CtrButton {
            text: "FLAPS"
            fact: f_flaps
            onTriggered: fact.value=highlighted?0:1
            highlighted: fact.value>0
        }
    }
    Component {
        id: btnAIRBRK
        CtrButton {
            text: "AIRBR"
            fact: f_airbrk
            onTriggered: fact.value=highlighted?0:1
            highlighted: fact.value>0
        }
    }
    Component {
        id: btnBRK_TAXI
        CtrButton {
            text: "BRAKE"
            fact: f_brake
            readonly property real v: fact.value
            color: (v>0 && v<1)?Qt.darker(Material.color(Material.Orange),1.5):undefined
            highlighted: v>0
            onTriggered: {
                if(m_reg_taxi){
                    f_brake.value=1
                }else{
                    f_brake.value=v>0?0:1
                }
            }
        }
    }
    Component {
        id: btnBRK
        CtrButton {
            text: "BRAKE"
            fact: f_brake
            onTriggered: fact.value=highlighted?0:1
            highlighted: fact.value>0
        }
    }
    Component {
        id: btnATAXI
        CtrButton {
            fact: f_action
            text: m_reg_taxi?"STOP":"AUTO"
            highlighted: m_reg_taxi
            onTriggered: fact.value = (m_reg_taxi?f_action.eval.reset:f_action.eval.next)
        }
    }
    Component {
        id: btnNEXT
        CtrButton {
            fact: f_action
            text: "NEXT"
            onTriggered: fact.value = f_action.eval.next
        }
    }
    Component {
        id: btnINC
        CtrButton {
            fact: f_action
            text: "NEXT"
            onTriggered: fact.value = f_action.eval.inc
        }
    }
    Component {
        id: btnDEC
        CtrButton {
            fact: f_action
            text: "PREV"
            onTriggered: fact.value = f_action.eval.dec
        }
    }
    Component {
        id: btnCANCEL
        CtrButton {
            fact: f_action
            text: "CANCEL"
            onTriggered: fact.value = f_action.eval.reset
        }
    }
    Component {
        id: btnRESET
        CtrButton {
            fact: f_action
            text: "RESET"
            onTriggered: fact.value = f_action.eval.reset
        }
    }
}
