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
    property var amsl: fact.parentFact.child("amsl").value
    property var coordinate: fact.parentFact.coordinate
    property var homeHmsl: mandala.est.ref.hmsl.value
    property var color: "#dcdcdc"
    property var elevation: NaN
    property var chosenFact: fact.parentFact.chosen
    property bool chosen: chosenFact == Waypoint.ALT
    
    anchors.fill: parent
    anchors.verticalCenter: parent.verticalCenter
    implicitHeight: parent.height
    implicitWidth: Math.max(icon.width+text.implicitWidth, height*4)
        
    // MaterialIcon {
    //     id: icon
    //     anchors.left: parent.left
    //     anchors.verticalCenter: parent.verticalCenter
    //     name: "home-map-marker"
    //     color: item.color
    //     size: text.contentHeight
    //     verticalAlignment: Text.AlignVCenter
    // }
    // Text {
    //     id: text
    //     anchors.left: icon.right
    //     anchors.verticalCenter: parent.verticalCenter
    //     verticalAlignment: Text.AlignVCenter
    //     font: apx.font_narrow(Style.fontSize)
    //     color: item.color
    //     text: getHomeHmsl()
    // }

    onChosenChanged: _editor.enabled = chosen
    onAglChanged: if(!chosen) altitudeProcessing()
    
    Component.onCompleted: {
        _editor.enabled = chosen
        if(visible)
            elevation = map.getElevationByCoordinate(coordinate)
        
        fact.parentFact.child("agl").visible = visible
    }

    function altitudeProcessing() 
    {   
        if(isNaN(elevation))
            return
        if(chosen)
            return
        
        var hAmsl = elevation + agl
        fact.value = amsl?hAmsl:hAmsl - homeHmsl 
    }

    function getHomeHmsl()
    {
        if(fact.parentFact.isFeets)
            return m2ft(homeHmsl) + "ft"
        return Math.round(homeHmsl) + "m"     
    }

    // Feets processing
    property var opts: fact.opts
    property var aglOpts: fact.parentFact.child("agl").opts

    onAglOptsChanged: if (!chosen) altitudeFtProcessing()

    function altitudeFtProcessing() {
        if(isNaN(elevation))
            return

        var hAmsl = parseInt(aglOpts.ft) +  m2ft(elevation)
        opts.ft = amsl?hAmsl:hAmsl - m2ft(homeHmsl)
        fact.opts = opts
    }
}
