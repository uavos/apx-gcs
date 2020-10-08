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

    delegate: Loader{
        asynchronous: true
        active: true
        visible: active
        width: listView.width
        height: active?MenuStyle.itemSize:0
        sourceComponent: Component {
            FactButton {
                size: MenuStyle.itemSize
                property var d: modelData
                text: d.title?d.title:qsTr("No title")
                descr: d.descr?d.descr:""
                value: d.value?d.value:""
                active: d.active?d.active:false
                showEditor: false
                onTriggered: {
                    parentFact.triggerItem(modelData)
                }
            }
        }
    }
}
