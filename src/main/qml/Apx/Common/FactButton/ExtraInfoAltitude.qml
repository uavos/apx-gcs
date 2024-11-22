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
import APX.Mission

Item {
    id: item
    visible: apx.settings.application.plugins.elevationmap.value && apx.tools.elevationmap.use.value

    property var map: apx.tools.elevationmap
    property var agl: fact.parentFact.child("agl").value
    property var hmsl: fact.parentFact.child("hmsl").value
    property var coordinate: fact.parentFact.coordinate
    property var homeHmsl: mandala.est.ref.hmsl.value
    property var color: "#dcdcdc"
    property var elevation: NaN
    property bool chosen: fact.parentFact.chosen == Waypoint.ALT

    anchors.fill: parent
    anchors.verticalCenter: parent.verticalCenter
    implicitHeight: parent.height
    implicitWidth: Math.max(icon.width+text.implicitWidth, height*4)
        
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
        text: homeHmsl + "m"
    }

    onChosenChanged: _editor.enabled = chosen
    onAglChanged: altitudeProcessing()
    onHmslChanged: altitudeProcessing()
    
    Component.onCompleted: {
        _editor.enabled = chosen
        if(visible)
            elevation = map.getElevationByCoordinate(coordinate)
    }

    function altitudeProcessing() 
    {   
        if(isNaN(elevation))
            return
        if(chosen)
            return
        if (fact.parentFact.chosen == Waypoint.AGL)
            fact.value = elevation + agl - homeHmsl
        if (fact.parentFact.chosen == Waypoint.HMSL) 
            fact.value = hmsl - homeHmsl
    }
}
