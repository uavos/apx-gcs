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
import QtQuick.Controls
import QtQuick.Controls.Material

import QtQml.Models

import Apx.Common
import APX.Fleet as APX
import APX.Mission

Item {
    id: item

    readonly property APX.Unit unit: apx.fleet.current
    readonly property Mission mission: unit.mission

    property var elevationmap: apx.tools.elevationmap
    property var plugin: apx.settings.application.plugins.elevationmap
    property var use: elevationmap ? apx.tools.elevationmap.use.value : false
    property var pluginOn: plugin ? apx.settings.application.plugins.elevationmap.value : false
    property var homeHmsl: mission.startElevation ? mission.startElevation : 0
    property var color: "#dcdcdc"
    property var chosenFact: fact.parentFact.chosen
    property bool chosen: chosenFact == Waypoint.ALT
    
    anchors.fill: parent
    anchors.verticalCenter: parent.verticalCenter
    implicitHeight: parent.height
    implicitWidth: Math.max(icon.width+text.implicitWidth, height*4)
    visible: !fact.parentFact.amsl.value && use && pluginOn
        
    MaterialIcon {
        id: icon
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        name: "home-map-marker"
        color: item.color
        size: text.contentHeight
        verticalAlignment: Text.AlignVCenter
    }
    Text {
        id: text
        anchors.left: icon.right
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        font: apx.font_narrow(Style.fontSize)
        color: item.color
        text: getHomeHmsl()
    }

    onChosenChanged: _editor.enabled = chosen

    Component.onCompleted: _editor.enabled = chosen

    function getHomeHmsl()
    {
        if(fact.parentFact.isFeets)
            return m2ft(homeHmsl) + "ft"
        return Math.round(homeHmsl) + "m"     
    }
}
