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

import APX.Vehicles as APX
import Apx.Common

ValueButton {
    id: control

    fact: apx.datalink

    text: qsTr("DL")
    toolTip: apx.datalink.descr+":"
           + "\n" + qsTr("Received packets")
           + "\n" + qsTr("Stream errors")
           + "\n" + qsTr("Transmitted packets")


    active: !(apx.datalink.ports.active || apx.datalink.hosts.active)
    warning: apx.datalink.readonly.value

    property bool light: active||warning

    enabled: true
    onPressAndHold: if(vehicle.protocol)vehicle.protocol.errcnt=0

    readonly property APX.Vehicle vehicle: apx.vehicles.current
    readonly property int errcnt: vehicle.protocol?vehicle.protocol.errcnt:0

    readonly property color cGreen: light?Material.color(Material.Yellow):Material.color(Material.LightGreen)
    readonly property color cRed: light?Material.color(Material.Yellow):Material.color(Material.DeepOrange)
    readonly property color cYellow: Material.color(Material.Amber)
    readonly property color cCyan: light?"#fff":Material.color(Material.Cyan)
    readonly property color cGrey: Material.color(Material.Grey)

    valueScale: 0.85
    
    valueC: Component {
        Row {
            spacing: 0
            layoutDirection: Qt.RightToLeft
            Text {
                height: parent.height
                font: apx.font_narrow(valueSize)
                verticalAlignment: Text.AlignVCenter
                text: "0%1".arg(apx.datalink.stats.uplink.cnt.value % 100).slice(-2)+" "
                color: apx.datalink.hbeat.value?cGreen:cGrey
            }
            Text {
                height: parent.height
                font: apx.font_narrow(valueSize)
                verticalAlignment: Text.AlignVCenter
                property int value: errcnt%10
                text: value+" "
                color: errcnt>1?(errTimer.running?cRed:cYellow):cGrey
                Behavior on color { enabled: ui.smooth; ColorAnimation {duration: 250} }
                Timer {
                    id: errTimer
                    interval: 5000
                    repeat: false
                }
                onValueChanged: errTimer.restart()
            }
            Text {
                height: parent.height
                font: apx.font_narrow(valueSize)
                verticalAlignment: Text.AlignVCenter
                text: "0%1".arg(apx.datalink.stats.dnlink.cnt.value%100).slice(-2)+" "
                color: apx.datalink.online?(vehicle.streamType===APX.PVehicle.TELEMETRY?cGreen:cCyan):cRed
            }
        }
    }
}
