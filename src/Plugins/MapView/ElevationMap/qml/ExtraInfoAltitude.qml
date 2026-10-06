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

    readonly property var missionTerrain: apx.tools.elevationmap.missionTerrain(mission)
    property var homeHmsl: (missionTerrain && missionTerrain.startElevation) ? missionTerrain.startElevation : 0
    property var color: "#dcdcdc"
    
    anchors.fill: parent
    anchors.verticalCenter: parent.verticalCenter
    implicitHeight: parent.height
    implicitWidth: Math.max(icon.width+text.implicitWidth, height*4)
    visible: !fact.parentFact.amsl.value
        
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

    function getHomeHmsl()
    {
        return Math.round(homeHmsl) + "m"     
    }
}
