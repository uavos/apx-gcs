import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0
import Apx.Application 1.0
import Apx.Menu 1.0

import APX.Facts 1.0

import "./lib"

ColumnLayout {
    id: control

    property var plugin: apx.tools.mapview
    property var factAdd: plugin.add
    property var factVehicle: plugin.vehicle

    Component.onCompleted: {
        ui.main.add(add, GroundControl.Layout.ToolBar)
        ui.main.add(vehicle, GroundControl.Layout.ToolBar)
        ui.main.add(tool, GroundControl.Layout.ToolBar)
        ui.main.add(control, GroundControl.Layout.Tool)
    }




    property var selectedTool: ui.map.selectedTool
    property var selectedGroup

    function selectTool(fact)
    {
        ui.map.selectedTool=fact
        selectedGroup=null
    }
    function reset()
    {
        selectTool(null)
    }
    onSelectedGroupChanged: {
        if(selectedGroup)
            ui.map.selectedTool=null
    }

    readonly property bool selGroup: selectedGroup?true:false
    readonly property bool selTool: selectedTool?true:false

    Repeater {
        visible: selGroup
        model: selectedGroup ? selectedGroup.model : null
        delegate: FactButton {
            fact: model.fact
            noFactTrigger: true
            onTriggered: selectTool(fact)
        }
    }


    Connections {
        target: ui.map
        enabled: selGroup
        onClicked: reset()
        onMoved: reset()
    }
    Connections {
        target: apx
        enabled: selTool || selGroup
        onFactTriggered: if(fact!=selectedTool) reset()
    }
    Connections {
        target: apx.vehicles
        enabled: selTool || selGroup
        onVehicleSelected: reset()
    }


    Loader {
        active: selTool
        asynchronous: true
        sourceComponent: Component {
            MapQuickItem {
                coordinate: ui.map.mouseCoordinate

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
            ui.map.addMapItem(item)
        }
    }


    FactButton {
        id: add
        fact: factAdd
        noFactTrigger: true
        showText: false
        highlighted: selectedGroup==fact
        onTriggered: {
            if(highlighted)reset()
            else selectedGroup=fact
        }
    }
    FactButton {
        id: vehicle
        fact: factVehicle
        noFactTrigger: true
        showText: false
        highlighted: selectedGroup==fact
        onTriggered: {
            if(highlighted)reset()
            else selectedGroup=fact
        }
    }
    Loader {
        id: tool
        active: selTool
        asynchronous: true
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
