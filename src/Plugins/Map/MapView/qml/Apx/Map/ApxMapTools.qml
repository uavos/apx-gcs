import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

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
    Repeater {
        model: selectedFact
               ? null
               : add.highlighted
                 ? factAdd.model
                 : vehicle.highlighted
                   ? factVehicle.model
                   : null
        delegate: FactButton {
            fact: model.fact
            noFactTrigger: true
            onTriggered: selectedFact=highlighted?null:fact
            highlighted: selectedFact==fact
        }
    }

    Connections {
        target: ui.map
        onClicked: {
            if(selectedFact){
                selectedFact.trigger()
            }else reset()
        }
        onMoved: {
            if(!selectedFact) reset()
        }
    }

    function reset()
    {
        selectedFact=null
        add.highlighted=false
        vehicle.highlighted=false
    }


    property bool sel: selectedFact?true:false
    FactButton {
        id: add
        fact: (highlighted && sel)?selectedFact:control.factAdd
        noFactTrigger: true
        showText: highlighted && sel
        onTriggered: {
            vehicle.highlighted=false
            selectedFact=null
            highlighted=!highlighted
        }
    }
    FactButton {
        id: vehicle
        fact: (highlighted && sel)?selectedFact:control.factVehicle
        noFactTrigger: true
        showText: highlighted && sel
        onTriggered: {
            add.highlighted=false
            selectedFact=null
            highlighted=!highlighted
        }
    }


}
