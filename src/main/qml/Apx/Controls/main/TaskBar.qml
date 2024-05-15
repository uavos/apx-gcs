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
import APX.Vehicles as APX

RowLayout {
    id: control

    readonly property APX.Vehicle vehicle: apx.vehicles.current

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
        asynchronous: true
        active: typeof(apx.tools)!=='undefined' && typeof(apx.tools.simulator)!=='undefined' && (vehicle.isLocal || apx.tools.simulator.stop.enabled || vehicle.title==="SIM")
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
        active: vehicle.isReplay
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
