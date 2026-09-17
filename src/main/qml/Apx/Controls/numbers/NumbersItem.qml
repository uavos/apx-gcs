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
import QtQuick.Controls.Material

import Apx.Common

ValueButton {
    id: control
    property bool light: false
    property bool fixedWidth: false

    alerts: true
    normalColor: light?"#555":normalColor
    activeColor: Qt.darker(Material.color(Material.LightBlue),1.5)



    property string title: fact?fact.name:""
    text: title

    // reserve space for minus sign, so the width doesn't jump when value sign changes
    readonly property real signWidth: /^\d/.test(value)?_signMetrics.advanceWidth:0
    property TextMetrics _signMetrics: TextMetrics {
        font: apx.font_narrow(control.valueSize)
        text: "-"
    }
    defaultWidth: Math.min(maximumWidth, Math.max(height, Math.max(minimumWidth, implicitContentWidth + signWidth + leftPadding+rightPadding)))

    //ensure width only grows
    Component.onCompleted: {
        if(fixedWidth){
            implicitWidth=height
        }else{
            implicitWidth=defaultWidth
        }
        updateWidth()
    }

    /*Connections {
        id: _con
        target: control
        //enabled: false
        function onDefaultWidthChanged(){ updateWidth() }
        function onModelChanged(){ updateWidth() }
    }*/

    function updateWidth()
    {
        if(fixedWidth){
            if(implicitWidth<defaultWidth){
                implicitWidth=Qt.binding(function(){return defaultWidth})
            }
            if(model && model.minimumWidth<defaultWidth){
                model.minimumWidth=defaultWidth
            }
        }else{
            if(implicitWidth<defaultWidth){
                implicitWidth=defaultWidth
            }
        }
    }

    onDefaultWidthChanged: timerWidthUpdate.start()
    property Timer timerWidthUpdate: Timer {
        //running: true
        interval: 100
        onTriggered: {
            updateWidth()
            interval=0
        }
    }

    //update model minimum width
    property var model
    onModelChanged: if(model && fixedWidth){
        minimumWidth=Qt.binding(function(){return model.minimumWidth})
        timerWidthUpdate.start()
    }
}
