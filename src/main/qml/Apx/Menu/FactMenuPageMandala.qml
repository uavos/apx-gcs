import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

import APX.Facts 1.0

FactMenuPageList {
    id: control
    property var parentFact: fact

    property var mandala: apx.vehicles.current.mandalatree


    Component.onCompleted: mandala.model.resetFilter()

    filterModel: true
    model: mandala.model

    function setValue(v)
    {
        parentFact.setValue(v)
        if(v && !parentFact.text){
            //no mandala - save text name only
            parentFact.statusText=v
            parentFact.valueChanged()
        }
    }

    delegate: FactButton {
        fact: modelData
        showEditor: false
        selected: fact.title===parentFact.text
        height: visible?MenuStyle.itemSize:0
        width: listView.width
        noFactTrigger: true
        /*onTriggered: {
            control.setValue(modelData.title)
            back()
        }*/
        //visible: fact && fact.visible && (control.filter=="" || fact.title.toLowerCase().includes(control.filter.toLowerCase()))
    }

    Fact {
        id: actionsFact
        flags: Fact.Group
        Fact {
            title: qsTr("Remove")
            flags: (Fact.Action | Fact.Remove)
            onTriggered: {
                control.setValue("")
                factMenu.back()
            }
        }
    }
    actionsModel: actionsFact.actionsModel

    /*Timer {
        id: posTimer
        interval: 100
        onTriggered: updateIndex()
    }

    property int count: model.count
    onCountChanged: posTimer.start() //updateIndex()
    function updateIndex()
    {
        headerItem.forceActiveFocus()
        for(var i=0;i<mandala.size;++i){
            if(mandala.child(i).title!==parentFact.text)continue
            listView.positionViewAtIndex(i,ListView.Center)
            break
        }
    }*/
}
