import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.13

import Apx.Common 1.0

Item {
    id: frame
    property var plugin

    state: plugin.state

    states: [
        State {
            name: "maximized"
            PropertyChanges {
                target: frame
                visible: false
            }
        },
        State {
            name: "minimized"
            PropertyChanges {
                target: frame
                visible: plugin.active
                implicitWidth: 300
                implicitHeight: implicitWidth*3/4
            }
            PropertyChanges {
                target: plugin
                parent: content
                anchors.fill: content
            }
        }
    ]

    Component.onCompleted: {
        plugin.parent=content
        plugin.anchors.fill=content
    }

    readonly property bool minimized: state!="maximized"
    visible: plugin.active

    implicitWidth: plugin.implicitWidth
    implicitHeight: plugin.implicitHeight
    DropShadow {
        enabled: minimized && ui.effects>1
        anchors.fill: parent
        source: content.background
        samples: enabled?15:0
        color: "#000"
        cached: true
        radius: samples/2
    }
    Item {
        id: content
        anchors.fill: parent
        property alias background: bgRect
        Rectangle {
            id: bgRect
            anchors.fill: parent
            border.width: 0
            color: "#000"
            radius: minimized?5:0
        }
        layer.enabled: minimized && ui.effects>0
        layer.effect: OpacityMask { maskSource: bgRect }
        Loader {
            z: 9999
            anchors.left: parent.left
            anchors.top: parent.top
            active: plugin.state==="minimized"
            sourceComponent: CleanButton {
                color: "transparent"
                iconName: "fullscreen"
                toolTip: qsTr("Maximize view")
                onTriggered: plugin.state="maximized"
            }
        }
    }
}
