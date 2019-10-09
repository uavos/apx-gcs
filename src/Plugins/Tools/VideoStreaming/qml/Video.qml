import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window 2.12
import QtMultimedia 5.7
import GstPlayer 1.0

import Apx.Common 1.0
import OverlayAim 1.0
import OverlayVars 1.0

// sample stream:
// rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov

Rectangle {
    id: videoItem

    Component.onCompleted: {
        application.registerUiComponent(videoItem, "video")
    }

    color: "black"

    readonly property var plugin: apx.tools.videostreaming
    readonly property var overlay: plugin?plugin.tune.overlay:undefined
    readonly property bool running: plugin?plugin.tune.running.value:false
    readonly property bool recording: plugin?plugin.tune.record.value:false
    readonly property var scale: overlay?overlay.scale:0.1

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        source: plugin
        ColumnLayout {
            visible: running
            x: videoOutput.contentRect.x
            y: videoOutput.contentRect.y
            spacing: 0
            Repeater {
                model: plugin.tune.overlay.varnames
                Label {
                    property var v: apx.vehicles.current.mandala.findChild(modelData)
                    text: v.name + ": " + v.value.toFixed(5)
                    font.pixelSize: videoOutput.contentRect.height / 40
                    styleColor: "black"
                    style: Text.Outline
                }
            }
        }
        OverlayAim {
            visible: plugin.connectionState === GstPlayer.STATE_CONNECTED
            type: plugin.tune.overlay.aim.value
            scale: overlay.scale.value
            x: videoOutput.contentRect.x
            y: videoOutput.contentRect.y
            width: videoOutput.contentRect.width
            height: videoOutput.contentRect.height
        }
        OverlayVars {
            visible: plugin.connectionState === GstPlayer.STATE_CONNECTED
            topLeftVars: overlay.topLeftVars.value
            topCenterVars: overlay.topCenterVars.value
            topRightVars: overlay.topRightVars.value
            scale: overlay.scale.value
            x: videoOutput.contentRect.x
            y: videoOutput.contentRect.y
            width: videoOutput.contentRect.width
            height: videoOutput.contentRect.height
        }
    }

    Button {
        id: connectButton
        visible: !running
        anchors.centerIn: parent
        text: qsTr("connect")
        scale: ui.scale
        onClicked: plugin.tune.running.value = true
    }
    BusyIndicator {
        visible: plugin.connectionState === GstPlayer.STATE_CONNECTING
        anchors.centerIn: parent
        running: true
    }

    ColumnLayout {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 3
        spacing: 5

        CleanButton {
            visible: running
            iconName: "record-rec"
            iconColor: recording ? Material.color(Material.DeepOrange) : Material.primaryTextColor
            onTriggered: plugin.tune.record.value =! plugin.tune.record.value
        }
        CleanButton {
            visible: running
            iconName: "image"
            onTriggered: plugin.snapshot()
        }
        CleanButton {
            visible: running
            iconName: "cast-off"
            onTriggered: plugin.tune.running.value=false
        }

        Item {
            height: 16
            width: height
        }

        CleanButton {
            iconName: "tune"
            onTriggered: {
                plugin.tune.trigger()
            }
        }

        /*CleanButton {
            id: resizeButton
            iconName: checked ? "fullscreen-exit" : "fullscreen"
            checkable: true
            onCheckedChanged: {
                if(checked)
                    plugin.state = "big"
                else
                    plugin.state = "small"
            }
        }*/
    }
}
