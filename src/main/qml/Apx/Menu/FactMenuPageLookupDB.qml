import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQml 2.12

import APX.Facts 1.0

import Apx.Common 1.0

FactMenuPageList {
    id: control

    property var parentFact: fact

    property bool filterEnabled: (fact)?true:false

    model: fact.dbModel
    delegate: Loader{
        asynchronous: true
        active: true
        visible: active
        width: listView.width
        height: active?MenuStyle.itemSize:0
        sourceComponent: Component {
            FactButton {
                height: MenuStyle.itemSize
                property var d: modelData
                title: d.title?d.title:qsTr("No title")
                descr: d.descr?d.descr:""
                status: d.status?d.status:""
                active: d.active?d.active:false
                showEditor: false
                onTriggered: {
                    parentFact.triggerItem(modelData)
                }

            }
        }
    }

    //filter
    header: TextField {
        id: filterText
        z: 100
        enabled: filterEnabled
        visible: enabled
        width: listView.width
        height: filterEnabled?implicitHeight:0
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: MenuStyle.titleFontSize
        placeholderText: qsTr("Search")+"..."
        background: Rectangle{
            border.width: 0
            color: Material.background
        }
        text: filterEnabled?fact.filter:""
        onTextEdited: fact.filter=text
        Connections {
            //prefent focus change on model updates
            target: listView
            enabled: control.filterEnabled
            onCountChanged: filterText.forceActiveFocus()
        }
    }


    Timer {
        //initial focus
        interval: 500
        running: true
        onTriggered: headerItem.forceActiveFocus()
    }
    Component.onCompleted: {
        fact.filter=""
        headerItem.forceActiveFocus()
    }
}
