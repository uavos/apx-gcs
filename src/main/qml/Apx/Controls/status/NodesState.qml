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

import Apx.Common

Rectangle {
    border.width: 0
    color: "#000"
    implicitWidth: itemWidth
    Layout.margins: 1

    readonly property real maxItems: 17.5
    readonly property real aspectRatio: 5

    readonly property real itemWidth: height/maxItems*aspectRatio
    readonly property real itemHeight: itemWidth/aspectRatio

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        spacing: 0
        cacheBuffer: 0
        model: apx.vehicles.current.nodes.model
        snapMode: ListView.SnapToItem
        delegate: ValueButton {
            implicitWidth: itemWidth
            size: itemHeight
            fact: modelData
            toolTip: fact?fact.text+"\n"+fact.descr:""
            value: fact?fact.text:"" //.startsWith('[')?fact.size:""
            valueScale: 0.7
            valueColor: titleColor
            enabled: true
            onTriggered: {
                if(fact.active)apx.vehicles.current.nodes.request()
                else apx.vehicles.current.nodes.nstat()
            }
            textColor: {
                if(fact){
                    if(fact.modified)return "#ffa" //Material.color(Material.Yellow)
                    if(fact.reconf)return "#faa" //Material.color(Material.Red)
                }
                return "#fff"
            }
        }
        headerPositioning: ListView.OverlayHeader
        header: ValueButton {
            z: 100
            implicitWidth: itemWidth
            size: itemHeight
            fact: apx.vehicles.current.nodes
            value: fact.text
            valueScale: 0.7
            enabled: true
            onTriggered: {
                if(fact.progress<0)fact.request()
                else fact.stop()
            }
        }
        ScrollBar.vertical: ScrollBar { width: 6 }
    }
}
