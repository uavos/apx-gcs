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
    }
    Item {
        id: systemBattery

        readonly property int level: apx.batteryLevel

        Layout.fillHeight: true
        Layout.preferredWidth: height * 0.95
        Layout.rightMargin: height * 0.25
        visible: apx.settings.graphics.systemBattery.value && apx.batteryAvailable

        Rectangle {
            id: batteryBody

            width: parent.width * 0.86
            height: parent.height * 0.90

            anchors.centerIn: parent

            radius: Math.max(2, width * 0.08)

            color: "transparent"
            border.width: Math.max(1, ui.scale)
            border.color: "#d8d8d8"

            Column {
                anchors.fill: parent
                anchors.margins: Math.max(2, batteryBody.border.width * 2)
                spacing: Math.max(1, ui.scale * 0.5)

                Repeater {
                    model: 10

                    Rectangle {
                        required property int index

                        width: parent.width
                        height: (parent.height - parent.spacing * 9) / 10

                        radius: Math.max(1, ui.scale)

                        color: systemBattery.level >= (10 - index) * 10
                               ? "#4caf50"
                               : "#28402a"
                    }
                }
            }

            Text {
                visible: apx.batteryCharging

                anchors.fill: parent

                text: "\u26A1"

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                color: "white"
                opacity: 0.55

                font.pixelSize: parent.height * 0.86
                fontSizeMode: Text.Fit

                style: Text.Outline
                styleColor: "#90000000"

                z: 10
            }

            Text {
                anchors.centerIn: parent

                text: systemBattery.level

                color: "white"

                font: apx.font_narrow(
                          Math.max(12, batteryBody.width * 0.58),
                          true)

                style: Text.Outline
                styleColor: "#c0000000"

                z: 20
            }
        }

        Rectangle {
            width: batteryBody.width * 0.42
            height: Math.max(2, parent.height * 0.065)

            anchors.horizontalCenter: batteryBody.horizontalCenter
            anchors.bottom: batteryBody.top

            color: batteryBody.border.color

            radius: Math.max(1, ui.scale)
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
