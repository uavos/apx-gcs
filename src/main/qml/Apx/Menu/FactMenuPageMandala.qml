import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

FactMenuListView {
    id: listView
    property var mandala: apx.vehicles.local.mandala
    property var parentFact: fact
    property string filter: ""

    model: mandala.model
    //currentIndex: fact
    delegate: FactButton {
        fact: modelData
        showEditor: false
        selected: fact.title===parentFact.text
        factTrigger: false
        height: visible?MenuStyle.itemSize:0
        width: listView.width
        onTriggered: {
            parentFact.setValue(modelData.title)
            factMenu.back()
        }
        visible: fact && fact.visible && (listView.filter=="" || fact.title.toLowerCase().includes(listView.filter.toLowerCase()))
    }
    contentsActions: [
        FactMenuAction {
            z: 10
            showText: true
            bRemove: true
            text: qsTr("Remove")
            iconName: "delete"
            visible: parentFact.text
            onTriggered: {
                parentFact.value=""
                factMenu.back()
            }
        }
    ]
    headerPositioning: ListView.OverlayHeader
    header: TextField {
        z: 100
        width: listView.width
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: MenuStyle.titleFontSize
        placeholderText: qsTr("Search")+"..."
        background: Rectangle{
            border.width: 0
            color: Material.background
        }
        onTextChanged: listView.filter=text.trim()
        onAccepted: {
            var f=mandala.findChild(text,false)
            //console.log(f)
            if(f){
                parentFact.setValue(f.title)
                factMenu.back()
            }
        }
    }
    Timer {
        id: posTimer
        interval: 100
        onTriggered: updateIndex()
    }
    onCountChanged: posTimer.start() //updateIndex()
    function updateIndex()
    {
        headerItem.forceActiveFocus()
        for(var i=0;i<mandala.size;++i){
            if(mandala.child(i).title!==parentFact.text)continue
            positionViewAtIndex(i,ListView.Center)
            break
        }
    }
}
