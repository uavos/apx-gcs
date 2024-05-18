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

import QtLocation
import QtPositioning

import Apx.Common
import Apx.Application

Item {
    id: control

    implicitHeight: buttons.implicitHeight
    implicitWidth: buttons.implicitWidth

    property var plugin: apx.tools.missionplanner
    property var factAdd: plugin.add
    property var factVehicle: plugin.vehicle

    property var selectedTool: map.selectedTool
    property var selectedGroup

    function selectTool(fact)
    {
        map.selectedTool=fact
        selectedGroup=null
    }
    function reset()
    {
        selectTool(null)
    }
    onSelectedGroupChanged: {
        if(selectedGroup)
            map.selectedTool=null
    }

    readonly property bool selGroup: selectedGroup?true:false
    readonly property bool selTool: selectedTool?true:false

    Connections {
        target: map
        enabled: selGroup
        function onClicked(){ reset() }
        function onMoved(){ reset() }
    }
    Connections {
        target: apx
        enabled: selTool || selGroup
        function onFactTriggered(fact){ if(fact!==selectedTool) reset() }
    }
    Connections {
        target: apx.vehicles
        enabled: selTool || selGroup
        function onVehicleSelected(){ reset() }
    }


    Loader {
        active: selTool
        visible: active
        // asynchronous: true
        sourceComponent: Component {
            MapQuickItem {
                coordinate: map.mouseCoordinate

                //constants
                anchorPoint.x: icon.implicitWidth/2
                anchorPoint.y: icon.implicitHeight*0.85

                sourceItem: MaterialIcon {
                    id: icon
                    size: 60
                    name: "chevron-double-down"
                    MaterialIcon {
                        //anchors.bottom: parent.top
                        anchors.verticalCenterOffset: -parent.implicitHeight/2
                        anchors.centerIn: parent
                        //size: 60
                        name: selectedTool.icon
                    }
                }
            }
        }
        onLoaded: {
            map.addMapItem(item)
        }
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.bottom
        anchors.topMargin: Style.buttonSize/4
        visible: selGroup
        spacing: Style.spacing*2
        Repeater {
            model: selectedGroup ? selectedGroup.model : null
            delegate: ActionButton {
                fact: model.fact
                noFactTrigger: true
                onTriggered: selectTool(fact)
            }
        }
    }

    Row {
        id: buttons
        spacing: Style.spacing*2
        
        ActionButton {
            id: add
            fact: factAdd
            noFactTrigger: true
            showText: false
            highlighted: selectedGroup===fact
            onTriggered: {
                if(highlighted)reset()
                else selectedGroup=fact
            }
        }
        ActionButton {
            id: vehicle
            fact: factVehicle
            noFactTrigger: true
            showText: false
            highlighted: selectedGroup===fact
            onTriggered: {
                if(highlighted)reset()
                else selectedGroup=fact
            }
        }
        Loader {
            id: tool
            active: selTool
            visible: active
            sourceComponent: Component {
                FactButton {
                    fact: selectedTool
                    noFactTrigger: true
                    showText: true
                    signaled: true
                    highlighted: true
                    onTriggered: reset()
                }
            }
        }
    }
}
