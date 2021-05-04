/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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

    focus: true

    implicitWidth: Style.buttonSize*10
    implicitHeight: implicitWidth*3/4

    Component.onCompleted: {
        application.registerUiComponent(control, "video")
    }

    readonly property var plugin: apx.tools.videostreaming
    readonly property var overlay: plugin?plugin.tune.overlay:undefined
    readonly property bool running: plugin?plugin.tune.running.value:false
    readonly property bool recording: plugin?plugin.tune.record.value:false
    readonly property var scale: overlay?overlay.scale:0.1

    readonly property int viewMode: plugin?plugin.tune.view_mode.value:false
    readonly property bool viewFull: viewMode>1

    readonly property bool alive: plugin.connectionState === GstPlayer.STATE_CONNECTED


    background: Rectangle {
        border.width: 0
        color: "#000"
        Item {
            anchors.fill: parent
            anchors.leftMargin: viewFull?0:control.leftPadding
            anchors.rightMargin: viewFull?0:control.rightPadding
            anchors.topMargin: viewFull?0:control.topPadding
            anchors.bottomMargin: viewFull?0:control.bottomPadding

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                source: plugin
                flushMode: VideoOutput.EmptyFrame
                fillMode: viewMode>0?VideoOutput.PreserveAspectCrop:VideoOutput.PreserveAspectFit

                layer.enabled: ui.effects
                layer.effect: ShaderEffect {
                    fragmentShader: Qt.resolvedUrl("/shaders/vignette.fsh")
                }
            }

            Overlay {
                id: overlay
                anchors.fill: videoOutput
                anchors.topMargin: viewFull?control.topPadding:0
                showNumbers: !pluginMinimized
                showAim: !pluginMinimized
                interactive: true
                frameRect: videoOutput.contentRect
                alive: control.alive
            }
        }
        Loader {
            anchors.centerIn: parent
            active: plugin.connectionState === GstPlayer.STATE_CONNECTING
            sourceComponent: BusyIndicator { }
        }
    }

    contentItem: Item {
        ColumnLayout {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: Style.spacing
            spacing: 5
            IconButton {
                visible: running
                iconName: "record-rec"
                iconColor: recording ? Material.color(Material.DeepOrange) : Material.primaryTextColor
                onTriggered: plugin.tune.record.value =! plugin.tune.record.value
            }
            IconButton {
                visible: running
                iconName: "image"
                onTriggered: plugin.snapshot()
            }
            IconButton {
                toolTip: plugin.tune.running.descr
                iconName: running?"cast-off":"cast"
                onTriggered: plugin.tune.running.value=!plugin.tune.running.value
            }

            Item {
                height: 16
                width: height
            }

            IconButton {
                iconName: "tune"
                onTriggered: {
                    plugin.tune.trigger()
                }
            }
        }
    }
}
