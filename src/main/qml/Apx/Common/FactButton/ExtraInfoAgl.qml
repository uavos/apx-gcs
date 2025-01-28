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
    property var map: apx.tools.elevationmap
    property var altitude: fact.parentFact.child("altitude").value
    property var amsl: fact.parentFact.child("amsl").value
    property var homeHmsl: mandala.est.ref.hmsl.value
    property var coordinate: fact.parentFact.coordinate
    property var value: fact.value
    property var elevation: NaN
    property var color: isNaN(elevation) ? "#dc143c" : "#32cd32"
    property var chosenFact: fact.parentFact.chosen
    property bool chosen: chosenFact == Waypoint.AGL

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
        text: isNaN(item.elevation) ? "NO" : getElevation()
    }

    onValueChanged: factButton.color = fact.value < fact.parentFact.unsafeAgl ? Material.color(Material.Red) : action_color()
    onVisibleChanged: fact.parentFact.chosen = Waypoint.ALT
    onChosenChanged: _editor.enabled = chosen
    onElevationChanged: fact.enabled = !isNaN(elevation)
    onAltitudeChanged: if(!chosen) aglProcessing()
    onAmslChanged: {calcAgl(); calcAglFt()}
    
    Component.onCompleted: {
        _editor.enabled = chosen
        elevation = map.getElevationByCoordinate(coordinate)
        aglProcessing()
        
        // Feets processing
        aglFtProcessing()
    }

    function aglProcessing() 
    {   
        if(isNaN(elevation)) {
            fact.value = 0
            return
        }
        calcAgl()
    }

    function calcAgl() {
        var diff = altitude - elevation
        fact.value = amsl?diff:homeHmsl+diff
    }

    function getElevation()
    {
        if(fact.parentFact.isFeets)
            return m2ft(item.elevation) + "ft"
        return Math.round(item.elevation) + "m"     
    }

    // Feets processing
    property var opts: fact.opts
    property var altOpts: fact.parentFact.child("altitude").opts
    onAltOptsChanged: aglFtProcessing()

    function aglFtProcessing() {
        if(chosen)
            return

        if(isNaN(elevation)) {
            opts.ft = 0
            fact.opts = opts
            return
        }

        calcAglFt()
    }

    function calcAglFt() {
        var diff = parseInt(altOpts.ft) - m2ft(elevation)
        opts.ft = amsl?diff:m2ft(homeHmsl) + diff
        fact.opts = opts
    }

}
