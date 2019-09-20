import QtQuick 2.5

import Apx.Common 1.0

import QtQuick.Layouts 1.12

AppPlugin {
    id: plugin
    sourceComponent: Video {
        id: video
        visible: apx.tools.VideoStreaming.show_window.value
    }
    state: "none"

    states: [
        State {
            name: "big"
            AnchorChanges {
                target: plugin
                anchors.top: ui.main.containerMain.top
                anchors.bottom: ui.main.containerMain.bottom
                anchors.left: ui.main.containerMain.left
                anchors.right: ui.main.containerMain.right
            }
        },
        State {
            name: "small"
            AnchorChanges {
                target: plugin
                anchors.top: ui.main.containerTop.bottom
                anchors.bottom: ui.main.containerBottom.top
                anchors.right: ui.main.containerRight.left
                anchors.left: undefined
            }
            PropertyChanges {
                target: plugin
                width: plugin.height
            }
        }
    ]
    transitions: Transition {
        id: transition
        AnchorAnimation {
            duration: 200
        }
    }

    uiComponent: "main"
    onConfigure: {
        plugin.parent = ui.main.containerMain
        plugin.state = "small"
    }
}
