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

    readonly property int m_mode: mandala.cmd.proc.mode.value
    readonly property real m_adj: mandala.cmd.proc.adj.value

    readonly property var f_hmsl: mandala.est.pos.hmsl
    readonly property var f_eta: mandala.est.wpt.eta
    readonly property var f_dist: mandala.est.wpt.dist
    readonly property var f_fuel: mandala.est.sys.fuel
    readonly property var f_wpidx: mandala.cmd.proc.wp
    readonly property var f_loops: mandala.cmd.proc.loops
    readonly property var f_xtrack: mandala.est.wpt.xtrack
    readonly property var f_radius: mandala.cmd.pos.radius

    readonly property var f_agl: mandala.est.pos.agl
    readonly property int m_agl_src: mandala.sns.agl.src.value
    readonly property bool m_agl_status: mandala.sns.agl.status.value
    readonly property bool m_pwr_agl: mandala.ctr.pwr.agl.value
    readonly property bool m_ahrs_hagl: mandala.cmd.ahrs.hagl.value

    readonly property bool m_agl_show: (m_agl_status || m_ahrs_hagl || m_pwr_agl || m_agl_src)
    readonly property bool m_agl_ready: m_agl_status
    readonly property bool m_agl_warning: m_ahrs_hagl && !m_agl_status
    readonly property bool m_agl_failure: m_pwr_agl && !m_agl_src

    readonly property real m_cmd_lat: mandala.cmd.pos.lat.value
    readonly property real m_cmd_lon: mandala.cmd.pos.lon.value
    readonly property real wp_dist: (m_cmd_lat!=0 && m_cmd_lon!=0)
                                    ? apx.vehicles.current.coordinate.distanceTo(QtPositioning.coordinate(m_cmd_lat, m_cmd_lon))
                                    : 0

    readonly property int m_reg_pos: mandala.cmd.reg.pos.value
    readonly property bool m_reg_taxi: mandala.cmd.reg.taxi.value
    property bool isTrack: m_reg_taxi || m_reg_pos===reg_pos_track || m_reg_pos===reg_pos_runway || m_reg_pos===reg_pos_loiter

    border.width: 0
    color: "#000"
    implicitWidth: itemHeight*3.2
    implicitHeight: layout.implicitHeight

    readonly property real itemHeight: height/15//*ui.scale

    property bool isLanding:
        m_mode===proc_mode_LANDING ||
        m_mode===proc_mode_TAKEOFF ||
        m_mode===proc_mode_TAXI

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
            visible: ui.test || (m_mode===proc_mode_WPT || m_mode===proc_mode_STBY)
            Layout.fillWidth: true
            Layout.preferredHeight: itemHeight
        }
        ValueButton {
            text: qsTr("LPS")
            fact: f_loops
            visible: ui.test || (m_mode===proc_mode_STBY && fact.value>0)
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
            visible: ui.test || m_mode===proc_mode_STBY || m_mode===proc_mode_LANDING
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
