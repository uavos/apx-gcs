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
import QtQuick          2.12
import QtLocation       5.12
import QtPositioning    5.12

import Apx.Common 1.0

MapBase {
    id: map

    readonly property real fontSize: Style.fontSize

    property var mapPlugin: apx.tools.missionplanner

    //map item setup
    color: 'transparent' // Necessary to make this map transparent

    Connections {
        target: map
        function onCenterChanged(){ mpTimer.start() }
        function onZoomLevelChanged(){ mpTimer.start() }
    }

    property real metersToPixelsFactor: 1
    Timer {
        id: mpTimer
        interval: 500
        onTriggered: {
            var coord = toCoordinate(Qt.point(width,height/2))
            var dist = center.distanceTo(coord)
            metersToPixelsFactor=(width/2)/dist;
        }
    }

    //select tool to trigger on click
    property var selectedTool
    onClicked: {
        if(!selectedTool)return
        selectedTool.trigger()
    }


    //internal
    property real itemsScaleFactor: 1

    property var selectedObject

    //---------------------------
    //MapTools sync
    onMouseClickCoordinateChanged: {
        if(mapPlugin) mapPlugin.clickCoordinate=map.mouseClickCoordinate
    }
    onMenuRequested: {
        if(selectedTool){
            selectedTool=null
            return
        }
        if(!mapPlugin)return
        mapPlugin.trigger({"posXY": mouseClickPoint})
    }
    onCenterChanged: mapToolsUpdateTimer.start()
    onZoomLevelChanged: mapToolsUpdateTimer.start()
    Timer {
        id: mapToolsUpdateTimer
        interval: 500
        onTriggered: updateMapToolsArea()
    }
    function updateMapToolsArea()
    {
        mapPlugin.area=map.visibleRegion
        area=map.visibleRegion
    }
    property var area: QtPositioning.shape()

}
