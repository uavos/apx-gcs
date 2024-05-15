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
import QtLocation
import QtPositioning

import Apx.Map.Common
import Apx.Common

MapObject {
    id: missionObject

    property var fact: null

    visible: mission.visible //&& visibleOnMap

    draggable: selected

    Connections {
        target: fact?fact:null
        enabled: fact!==null
        function onTriggered(){ selectAndCenter() }
    }

    onTriggered: {
        if(fact) fact.trigger()
    }

    property int detailsLevel: 20
    Connections {
        target: map
        function onZoomLevelChanged(){ dlTimer.start() }
        function onSelectedObjectChanged() {
            if(!fact)return
            fact.selected = selected
        }
    }
    Timer {
        id: dlTimer
        interval: 1000
        onTriggered: detailsLevel=map.zoomLevel
    }

    Connections {
        target: mission
        function onSelectedItemChanged() {
            if(!fact)return
            if(mission.selectedItem === fact && !selected) selectAndCenter()
        }
    }

    contentsCenter: Loader {
        z: -1
        active: fact?fact.active:false
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: missionObject.width
        sourceComponent: Component {
            MaterialIcon {
                size: missionObject.width*1.8
                name: "chevron-left"
                color: missionObject.color
            }
        }
    }

    //Fact bindings
    title: fact?fact.num+1:0
    implicitCoordinate: fact?fact.coordinate:QtPositioning.coordinate()

    property real f_altitude: (fact && fact.altitude)?fact.altitude.value:0
    property bool active: fact?fact.active:false

    //dragging support
    onMoved: {
        if(fact){
            fact.coordinate=coordinate
        }
    }
}
