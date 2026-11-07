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
import QtQuick
import QtQuick.Layouts

import QtQml.Models

import Apx.Common
import APX.Fleet as APX

RowLayout {
    id: control

    readonly property APX.Unit unit: apx.fleet.current

    spacing: Style.spacing*2

    //warnings fact
    WarningMessage {
        Layout.fillHeight: true
    }

    //widgets
    WidgetsListView {
        id: widgetsView
        Layout.fillHeight: true
    }
    function addWidgetControl(plugin, index){widgetsView.add(plugin,index)}

    // FactButton {
    //     fact: apx.settings.graphics.scale
    // }


    //tools list from plugins
    DelegateModel {
        id: toolsModel
        model: apx.windows.model
        groups: [ DelegateModelGroup { name: "launcher" } ]
        filterOnGroup: "launcher"
        delegate: IconButton {
            iconName: modelData.icon
            onTriggered: modelData.trigger()
            toolTip: modelData.title+" - "+modelData.descr
        }
        Component.onCompleted: sync()
        property int size: apx.windows.model.count
        onSizeChanged: sync()
        function sync()
        {
            for( var i = 0;i < items.count;i++ ) {
                var entry = items.get(i)
                if(entry.model.modelData && entry.model.modelData.showLauncher) {
                    toolsModel.items.setGroups(i--,1,"launcher")
                }
            }
        }
    }
    ListView {
        Layout.fillHeight: true
        implicitWidth: contentWidth
        spacing: Style.spacing
        orientation: ListView.Horizontal
        model: toolsModel
    }

    //tools menu
    IconButton {
        Layout.fillHeight: true
        iconName: apx.tools.icon
        toolTip: apx.tools.title
        onTriggered: apx.tools.trigger()
    }
    //windows menu
    IconButton {
        Layout.fillHeight: true
        iconName: apx.windows.icon
        toolTip: apx.windows.title
        onTriggered: apx.windows.trigger()
    }

    //simulator
    Loader {
        Layout.fillHeight: true
        active: typeof(apx.tools)!=='undefined' && typeof(apx.tools.simulator)!=='undefined' && (unit.isLocal || apx.tools.simulator.stop.enabled || unit.title==="SIM")
        sourceComponent: Component {
            IconButton {
                iconName: apx.tools.simulator.icon
                toolTip: apx.tools.simulator.descr
                onTriggered: apx.tools.simulator.trigger()
            }
        }
        visible: status===Loader.Ready
    }

    IconButton {
        Layout.fillHeight: true
        iconName: groundControl.maximized?"fullscreen-exit":"fullscreen"
        toolTip: qsTr("Switch view")
        onTriggered: groundControl.toggleState()
    }

    Loader {
        Layout.alignment: Qt.AlignRight|Qt.AlignTop
        active: unit.isReplay
        visible: active
        sourceComponent: Component { TelemetryReader { } }
    }
    RecLabel {
        Layout.fillHeight: true
    }
    ClockLabel {
        Layout.fillHeight: true
        Layout.rightMargin: height*0.3
    }
    Loader {
        Layout.fillHeight: true
        active: apx.settings.graphics.systemBattery.value && apx.batteryAvailable
        visible: status === Loader.Ready
        sourceComponent: Component {
            // macOS-like horizontal battery with percentage on the right
            Row {
                id: systemBattery

                readonly property int level: apx.batteryLevel
                readonly property bool charging: apx.batteryCharging
                readonly property bool low: level <= 20 && !charging

                readonly property color frameColor: "#b0ffffff"
                readonly property color fillColor: low ? "#f44336" : "#ffffff"

                height: parent.height
                spacing: Math.max(2, height * 0.12)

                Item {
                    id: batteryIcon

                    height: parent.height
                    width: height * 1.1

                    Rectangle {
                        id: batteryBody

                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter

                        width: parent.width - batteryTip.width - batteryTip.anchors.leftMargin
                        height: parent.height * 0.5

                        radius: height * 0.22

                        color: "transparent"
                        border.width: Math.max(1, ui.scale)
                        border.color: systemBattery.frameColor

                        Rectangle {
                            id: batteryFill

                            readonly property real gap: Math.max(1.5, batteryBody.border.width * 1.5)

                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: gap

                            width: (batteryBody.width - gap * 2) * Math.max(0, Math.min(100, systemBattery.level)) / 100

                            radius: Math.max(1, batteryBody.radius - gap)

                            color: systemBattery.fillColor

                            Behavior on width { NumberAnimation { duration: 300 } }
                        }

                        // charging bolt (white, same shape as the emoji bolt)
                        Canvas {
                            id: batteryBolt

                            visible: systemBattery.charging

                            anchors.centerIn: parent

                            width: parent.height * 1.05
                            height: parent.height * 1.55

                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()
                            onVisibleChanged: if (visible) requestPaint()

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                var w = width, h = height
                                ctx.beginPath()
                                ctx.moveTo(w * 0.62, 0)
                                ctx.lineTo(w * 0.08, h * 0.58)
                                ctx.lineTo(w * 0.46, h * 0.58)
                                ctx.lineTo(w * 0.38, h)
                                ctx.lineTo(w * 0.92, h * 0.40)
                                ctx.lineTo(w * 0.54, h * 0.40)
                                ctx.closePath()
                                ctx.lineJoin = "round"
                                ctx.lineWidth = Math.max(1, ui.scale)
                                ctx.strokeStyle = "#c0000000"
                                ctx.fillStyle = "#ffffff"
                                ctx.fill()
                                ctx.stroke()
                            }
                        }
                    }

                    // terminal tip
                    Rectangle {
                        id: batteryTip

                        anchors.left: batteryBody.right
                        anchors.leftMargin: Math.max(1, ui.scale * 0.5)
                        anchors.verticalCenter: batteryBody.verticalCenter

                        width: Math.max(2, batteryBody.height * 0.16)
                        height: batteryBody.height * 0.4

                        radius: width * 0.5

                        color: systemBattery.frameColor
                    }
                }

                Text {
                    id: batteryText

                    height: parent.height
                    // reserve width for "100%" so the taskbar doesn't jump on level change
                    width: Math.max(implicitWidth, _batteryMetrics.advanceWidth)

                    text: systemBattery.level + "%"

                    verticalAlignment: Text.AlignVCenter

                    color: systemBattery.low ? systemBattery.fillColor : "#fff"

                    font: apx.font_narrow(parent.height * 0.8)

                    TextMetrics {
                        id: _batteryMetrics
                        font: batteryText.font
                        text: "100%"
                    }
                }
            }
        }
    }
    Loader {
        Layout.fillHeight: true
        active: Qt.platform.os === "linux"
        visible: active
        sourceComponent: Component {
            ActionButton {
                fact: apx.sysmenu
                showText: false
            }
        }
    }
}
