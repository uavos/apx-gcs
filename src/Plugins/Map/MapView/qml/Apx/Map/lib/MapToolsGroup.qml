import QtQuick          2.12
import QtQuick.Layouts  1.12

import Apx.Common 1.0
import Apx.Menu 1.0

import APX.Facts 1.0

ColumnLayout {
    id: control

    property var fact


    property var selectedFact
    Repeater {
        model: (button.highlighted && !selectedFact)?control.fact.model:null
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
            }else{
                button.highlighted=false
            }
        }
        onMoved: {
            if(!selectedFact)
                button.highlighted=false
        }
    }

    FactButton {
        id: button
        fact: selectedFact?selectedFact:control.fact
        noFactTrigger: true
        showText: selectedFact?true:false
        highlighted: false
        onTriggered: {
            selectedFact=null
            highlighted=!highlighted
        }
    }


}
