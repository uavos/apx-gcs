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

import Apx.Common

Item {
    id: item
    visible: apx.settings.application.plugins.elevationmap.value && apx.tools.elevationmap.use.value
    property var map: apx.tools.elevationmap
    property var altitude: fact.parentFact.child("altitude").value
    property var coordinate: fact.parentFact.coordinate
    property var elevation: NaN
    property var color: isNaN(elevation) ? "#dc143c" : "#32cd32"
    property var checked: fact.parentFact.isAgl

    // These parameters are not absolute, discussion is possible
    readonly property var altLimit: 100 // Suggested by the CEO
    property var homeHmsl: mandala.est.ref.hmsl.value

    anchors.fill: parent
    anchors.verticalCenter: parent.verticalCenter
    implicitHeight: parent.height
    implicitWidth: Math.max(icon.width+text.implicitWidth, height*4)
        
    MaterialIcon {
        id: icon
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        name: "elevation-rise"
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
        text: isNaN(item.elevation) ? "NO" : item.elevation + "m"
    }

    onAltitudeChanged: aglProcessing()
    onCheckedChanged: _editor.enabled = checked
    onVisibleChanged: fact.parentFact.isAgl = false
    
    Component.onCompleted: {
        _editor.enabled = checked
        elevation = map.getElevationByCoordinate(coordinate)
        aglProcessing() 
    }

    function aglProcessing() 
    {   
        if(isNaN(elevation)) {
            fact.value = 0
            return
        }

        fact.value = homeHmsl + altitude - elevation;
        factButton.color = fact.value < altLimit ? Material.color(Material.Red) : action_color()
        // factButton.color = fact.value < altitude * 0.1 ? Material.color(Material.Red) : action_color()
    }
}
