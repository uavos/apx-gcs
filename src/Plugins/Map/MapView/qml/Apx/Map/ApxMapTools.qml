import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0
import Apx.Menu 1.0

import APX.Facts 1.0

import "./lib"

ColumnLayout {
    id: control

    property var plugin: apx.tools.mapview
    property var factAdd: plugin.add
    property var factVehicle: plugin.vehicle

    Component.onCompleted: {
        ui.main.mainLayout.addItem(add,Qt.AlignLeft|Qt.AlignTop)
        ui.main.mainLayout.addItem(vehicle,Qt.AlignLeft|Qt.AlignTop)
        ui.main.mainLayout.addItem(control,Qt.AlignLeft|Qt.AlignVCenter)
    }



    property var selectedFact
    property var selectedGroup

    property bool active: activeTool || (selectedGroup?true:false)
    property bool activeTool: selectedFact?true:false

    Repeater {
        model: selectedFact ? null : selectedGroup ? selectedGroup.model : null
        delegate: FactButton {
            fact: model.fact
            noFactTrigger: true
            onTriggered: selectedFact=highlighted?null:fact
            highlighted: selectedFact==fact
        }
    }



    Connections {
        target: ui.map
        enabled: control.active
        onClicked: {
            if(selectedFact){
                selectedFact.trigger()
            }else reset()
        }
    }
    Connections {
        target: ui.map
        enabled: !activeTool
        onMoved: reset()
    }
    Connections {
        target: apx
        enabled: control.active
        onFactTriggered: {
            if(fact==selectedFact)return
            reset()
        }
    }
    Connections {
        target: apx.vehicles
        enabled: control.active
        onVehicleSelected: reset()
    }

    function reset()
    {
        selectedFact=null
        selectedGroup=null
    }

    Loader {
        active: activeTool
        asynchronous: true
        sourceComponent: Component {
            MapQuickItem {
                coordinate: ui.map.mouseCoordinate

                //constants
                anchorPoint.x: icon.implicitWidth/2
                anchorPoint.y: icon.implicitHeight*0.85

                sourceItem: Text {
                    id: icon
                    font.family: "Material Design Icons"
                    font.pixelSize: 60
                    text: materialIconChar["chevron-double-down"]
                    color: "#fff"
                }
            }
        }
        onLoaded: {
            ui.map.addMapItem(item)
        }
    }


    FactButton {
        id: add
        fact: (highlighted && activeTool)?selectedFact:control.factAdd
        noFactTrigger: true
        showText: highlighted && activeTool
        signaled: showText
        highlighted: selectedGroup==factAdd
        onTriggered: {
            if(highlighted)reset()
            else{
                if(activeTool)reset()
                selectedGroup=factAdd
            }
        }
    }
    FactButton {
        id: vehicle
        fact: (highlighted && activeTool)?selectedFact:control.factVehicle
        noFactTrigger: true
        showText: highlighted && activeTool
        signaled: showText
        highlighted: selectedGroup==factVehicle
        onTriggered: {
            if(highlighted)reset()
            else{
                if(activeTool)reset()
                selectedGroup=factVehicle
            }
        }
    }
}
