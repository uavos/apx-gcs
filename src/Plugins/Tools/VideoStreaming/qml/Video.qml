import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtMultimedia 5.13
import QtQuick.Controls.Material 2.12

import GstPlayer 1.0

import Apx.Common 1.0

// sample stream:
// rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov

Control {
    id: control

    Component.onCompleted: {
        application.registerUiComponent(control, "video")
        //ui.main.add(control, GroundControl.Layout.Main, 0)
    }

    readonly property var plugin: apx.tools.videostreaming
    readonly property var overlay: plugin?plugin.tune.overlay:undefined
    readonly property bool running: plugin?plugin.tune.running.value:false
    readonly property bool recording: plugin?plugin.tune.record.value:false
    readonly property var scale: overlay?overlay.scale:0.1

    readonly property bool viewMode: plugin?plugin.tune.view_mode.value:false

    contentItem: Rectangle {

        implicitWidth: 400
        implicitHeight: implicitWidth*3/4

        border.width: 0
        color: "#000"
        radius: 5

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            source: plugin
            flushMode: VideoOutput.EmptyFrame
            fillMode: control.viewMode?VideoOutput.PreserveAspectCrop:VideoOutput.PreserveAspectFit

            /*OverlayAim {
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
                topLeftVars: overlay.top_left_vars.value
                topCenterVars: overlay.top_center_vars.value
                topRightVars: overlay.top_right_vars.value
                scale: overlay.scale.value
                x: videoOutput.contentRect.x
                y: videoOutput.contentRect.y
                width: videoOutput.contentRect.width
                height: videoOutput.contentRect.height
            }
            OverlayGimbal {
                visible: plugin.connectionState === GstPlayer.STATE_CONNECTED && overlay.show_gimbal.value === true
                yawVar: overlay.gimbal_yaw_var.value
                pitchVar: overlay.gimbal_pitch_var.value
                scale: overlay.scale.value
                x: videoOutput.contentRect.x
                y: videoOutput.contentRect.y
                width: videoOutput.contentRect.width
                height: videoOutput.contentRect.height
            }*/

            Overlay {
                anchors.fill: parent
                visible: !pluginMinimized
                interactive: true
                frameRect: videoOutput.contentRect
                alive: plugin.connectionState === GstPlayer.STATE_CONNECTED
            }
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
                toolTip: plugin.tune.running.descr
                iconName: running?"cast-off":"cast"
                onTriggered: plugin.tune.running.value=!plugin.tune.running.value
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
}
