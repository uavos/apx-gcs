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
import QtQuick.Controls
import QtQuick.Controls.Material
import QtPositioning

import Apx.Common

Rectangle {

    readonly property var f_mode: mandala.fact("cmd.proc.mode")
    readonly property real m_adj: mandala.fact("cmd.proc.adj").value

    readonly property var f_hmsl: mandala.fact("est.pos.hmsl")
    readonly property var f_eta: mandala.fact("est.wpt.eta")
    readonly property var f_dist: mandala.fact("est.wpt.dist")
    readonly property var f_fuel: mandala.fact("est.sys.fuel")
    readonly property var f_wpidx: mandala.fact("cmd.proc.wp")
    readonly property var f_loops: mandala.fact("cmd.proc.orbs")
    readonly property var f_xtrack: mandala.fact("est.wpt.xtrack")
    readonly property var f_radius: mandala.fact("cmd.pos.radius")

    readonly property var f_agl: mandala.fact("est.pos.agl")
    readonly property int m_agl_src: mandala.fact("sns.agl.src").value
    readonly property bool m_agl_status: mandala.fact("sns.agl.status").value
    readonly property bool m_pwr_agl: mandala.fact("ctr.pwr.agl").value
    readonly property bool m_ins_hagl: mandala.fact("cmd.ins.hagl").value

    readonly property bool m_agl_show: (m_agl_status || m_ins_hagl || m_pwr_agl || m_agl_src)
    readonly property bool m_agl_ready: m_agl_status
    readonly property bool m_agl_warning: m_ins_hagl && !m_agl_status
    readonly property bool m_agl_failure: m_pwr_agl && !m_agl_src

    readonly property real m_cmd_lat: mandala.fact("cmd.pos.lat").value
    readonly property real m_cmd_lon: mandala.fact("cmd.pos.lon").value
    readonly property real wp_dist: (m_cmd_lat!=0 && m_cmd_lon!=0)
                                    ? apx.fleet.current.coordinate.distanceTo(QtPositioning.coordinate(m_cmd_lat, m_cmd_lon))
                                    : 0

    readonly property var f_reg_hdg: mandala.fact("cmd.reg.hdg")
    readonly property var f_reg_taxi: mandala.fact("cmd.reg.taxi")
    property bool isTrack: f_reg_taxi.value > 0 || f_reg_hdg.value == f_reg_hdg.eval.track || f_reg_hdg.value == f_reg_hdg.eval.loiter

    border.width: 0
    color: "#000"
    implicitWidth: itemHeight*3.2
    implicitHeight: layout.implicitHeight

    readonly property real itemHeight: height/15//*ui.scale

    property bool isLanding:
        f_mode.value == f_mode.eval.LANDING ||
        f_mode.value == f_mode.eval.TAKEOFF ||
        f_mode.value == f_mode.eval.TAXI

    ProgressBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Style.spacing
        property int v: apx.progress
        to: 100
        value: v
        visible: v>=0
        indeterminate: v==0
        Material.accent: Material.color(Material.Green)
    }

    ColumnLayout{
        id: layout
        anchors.fill: parent
        spacing: 0

        ValueRss {
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        ValueDL {
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        Loader {
            asynchronous: true
            active: apx.datalink.server.text || ui.test
            visible: active
            sourceComponent: Component {
                ValueButton {
                    text: qsTr("RC")
                    fact: apx.datalink.server
                    active: false
                    valueScale: 0.8
                    valueColor: fact.extctr.value?Material.color(Material.LightGreen):Material.color(Material.Red)
                    enabled: true
                }
            }
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        Loader {
            asynchronous: true
            active: apx.datalink.hosts.text
            visible: active
            sourceComponent: Component {
                ValueButton {
                    text: qsTr("RS")
                    fact: apx.datalink.hosts
                    active: false
                    valueScale: 0.8
                    valueColor: apx.datalink.server.extctr.value?Material.color(Material.LightGreen):Material.color(Material.LightRed)
                    enabled: true
                }
            }
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        ValueLOS {
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }


        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        ValueButton {
            text: qsTr("H")
            fact: f_hmsl
            property real v: fact.value*(3.281/100)
            value: "FL"+v.toFixed()
            visible: ui.test || v>0
            valueScale: 0.8
            valueColor: Material.color(Material.BlueGrey)
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("D")
            fact: f_dist
            value: wp_dist>0?apx.distanceToString(wp_dist):"--"
            valueScale: 0.8
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("ETA")
            fact: f_eta
            property int v: fact.value
            property real valid: v>0
            property int tsec: ("0"+Math.floor(v%60)).slice(-2)
            property int tmin: ("0"+Math.floor(v/60)%60).slice(-2)
            property int thrs: Math.floor(v/60/60)
            property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
            value: valid?sETA:"--:--"
            valueScale: 0.8
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("F")
            fact: f_fuel
            value: fact.value.toFixed()
            visible: ui.test || fact.value>0
            valueScale: 0.8
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight/2
        }

        ValueButton {
            text: qsTr("WPT")
            fact: f_wpidx
            value: fact.value+1
            visible: ui.test || (f_mode.value == f_mode.eval.WPT || f_mode.value == f_mode.eval.STBY)
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("LPS")
            fact: f_loops
            visible: ui.test || (f_mode.value == f_mode.eval.STBY && fact.value>0)
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("XT")
            fact: f_xtrack
            visible: ui.test || isTrack
            property real v: fact.value
            value: (Math.abs(v)<1?0:v.toFixed())+(m_adj>0?"+"+m_adj.toFixed():m_adj<0?"-"+(-m_adj).toFixed():"")
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("LR")
            fact: f_radius
            visible: ui.test || f_mode.value == f_mode.eval.STBY || f_mode.value == f_mode.eval.LANDING
            value: apx.distanceToString(Math.abs(fact.value))
            valueScale: 0.8
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("AGL")
            fact: f_agl
            value: fact.value.toFixed(1)
            visible: ui.test || m_agl_show
            warning: m_agl_warning
            error: m_agl_failure
            active: m_agl_ready
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

    }
}
