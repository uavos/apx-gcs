import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

import APX.Facts 1.0

ColumnLayout {
    id: control

    property alias model: listView.model
    property alias delegate: listView.delegate
    property alias actionsModel: actionsListView.model

    property alias header: listView.header
    property alias headerItem: listView.headerItem

    property ListView listView: listView

    //facts
    FactMenuListView {
        id: listView
        Layout.fillHeight: true
        Layout.fillWidth: true
        delegate: Loader{
            asynchronous: true
            active: modelData && modelData.visible
            visible: active
            width: control.width
            height: active?MenuStyle.itemSize:0
            sourceComponent: Component {
                FactButton {
                    fact: modelData;
                    height: MenuStyle.itemSize
                    Connections {
                        target: modelData
                        onTriggered: if(factMenu) factMenu.factTriggered(modelData)
                    }
                    onTriggered: listView.currentIndex=index
                }
            }
            onLoaded: {
                if(index==0) item.focusRequested()
            }
        }
    }

    //actions
    ListView {
        id: actionsListView

        Layout.alignment: Qt.AlignRight
        Layout.bottomMargin: control.spacing

        implicitHeight: MenuStyle.itemSize
        implicitWidth: contentItem.childrenRect.width

        clip: true
        focus: true
        spacing: 5

        model: fact.actionsModel
        snapMode: ListView.SnapToItem
        orientation: ListView.Horizontal

        visible: count>0

        property bool actionsText: true
        delegate: Loader{
            asynchronous: true
            active: modelData && modelData.visible && ((modelData.options&Fact.ShowDisabled)?true:modelData.enabled)
            visible: active
            sourceComponent: Component {
                FactMenuAction {
                    fact: modelData;
                    showText: actionsListView.actionsText
                    Connections {
                        target: modelData
                        onTriggered: if(factMenu)factMenu.factTriggered(modelData)
                    }
                }
            }
        }
    }
}
