import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

FactMenuListView {
    id: listView
    delegate: Loader{
        asynchronous: true
        active: modelData && modelData.visible
        visible: active
        width: listView.width
        height: active?MenuStyle.itemSize:0
        sourceComponent: Component {
            FactButton {
                fact: modelData;
                height: MenuStyle.itemSize
                Connections {
                    target: modelData
                    onTriggered: if(factMenu)factMenu.factTriggered(modelData)
                }
            }
        }
        onLoaded: {
            if(index==0)item.focusRequested()
        }
    }
}
