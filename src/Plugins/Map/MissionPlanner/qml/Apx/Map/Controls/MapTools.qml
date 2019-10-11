import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0
import Apx.Application 1.0

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
        onClicked: reset()
        onMoved: reset()
    }
    Connections {
        target: apx
        enabled: selTool || selGroup
        onFactTriggered: if(fact!==selectedTool) reset()
    }
    Connections {
        target: apx.vehicles
        enabled: selTool || selGroup
        onVehicleSelected: reset()
    }


    Loader {
        active: selTool
        visible: active
        asynchronous: true
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

    ColumnLayout {
        anchors.left: parent.left
        anchors.top: parent.bottom
        anchors.topMargin: 8
        visible: selGroup
        Repeater {
            model: selectedGroup ? selectedGroup.model : null
            delegate: FactButton {
                fact: model.fact
                noFactTrigger: true
                onTriggered: selectTool(fact)
            }
        }
    }

    RowLayout {
        id: buttons
        FactButton {
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
        FactButton {
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
