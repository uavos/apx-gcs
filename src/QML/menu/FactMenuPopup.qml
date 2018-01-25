import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import "../components"
import "."

Popup {
    id: popupItem
    //modal: true
    //focus: true
    parent: window

    property alias fact: factMenu.fact
    property alias showTitle: factMenu.showTitle
    property bool closeOnTrigger: true

    //contentWidth: Math.min(window.width, window.height) / 3 * 2
    //contentHeight: Math.min(window.height, contents.length*menu.height)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    //contentHeight: factMenu.implicitHeight //12*32
    //contentWidth: factMenu.implicitWidth //32*10 //cWidth

    padding: 0
    margins: 0

    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: app.settings.smooth.value?0.9:1 } }

    contentItem: FactMenu {
        id: factMenu
        autoResize: true
        onFactPageRemoved: popupItem.close()
        onActionTriggered: if(closeOnTrigger)popupItem.close()
        Label {
            id: btnClose
            z: 10
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 4
            font.family: "Material Design Icons"
            font.pointSize: factMenu.titleSize*0.8
            text: materialIconChar["close"]
            background: Rectangle {
                anchors.fill: parent
                border.width: 0
                color: "#888"
                opacity: mouseArea.containsMouse?1:0
                Behavior on opacity { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: popupItem.close()
            }
        }
    }
}

