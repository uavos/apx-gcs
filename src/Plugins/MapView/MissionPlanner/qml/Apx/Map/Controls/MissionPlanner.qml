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
import QtQuick 2.12
import QtQuick.Layouts 1.12

import Apx.Application 1.0
import Apx.Map.MapView 1.0

import Apx.Common 1.0
import Apx.Controls 1.0

MapView {
    id: missionPlanner

    readonly property bool showWind: mandala.est.wind.status.value > 0 || mandala.est.wind.speed.value > 0

    readonly property real margins: Style.spacing

    //initial animation
    PropertyAnimation {
        running: true
        target: map
        property: "zoomLevel"
        from: 12
        to: 16.33
        duration: 1000
        easing.type: Easing.OutInCirc
    }

    Component.onCompleted: {
        application.registerUiComponent(map,"map")
        application.registerUiComponent(missionPlanner,"missionPlanner")
        //ui.main.add(control, GroundControl.Layout.Main, 1)
        //ui.main.add(main, GroundControl.Layout.Main)
    }
    onMapBackgroundItemLoaded: {
        application.registerUiComponent(item,"mapbase")
    }

    implicitWidth: 400
    implicitHeight: 400

    showNavigation: !pluginMinimized

    //Controls
    /*Item {
        z: 9999
        BoundingRect { item: toolBar }
        BoundingRect { item: missionList }
        BoundingRect { item: bottom }
        BoundingRect { item: info }
        BoundingRect { item: status }
    }*/


    contentItem: Item {
        id: main
        visible: showNavigation
        //anchors.fill: parent
        RowLayout {
            id: toolBar
            z: 100
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: Style.spacing*2
            MissionTools {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            }
            MapTools { }
        }

        RowLayout {
            id: missionList
            z: 50
            anchors.left: parent.left
            anchors.top: toolBar.bottom
            anchors.bottom: info.top
            anchors.topMargin: margins
            anchors.bottomMargin: margins
            spacing: Style.spacing
            MissionListView {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                Layout.fillHeight: true
            }
        }

        RowLayout {
            id: info
            anchors.left: parent.left
            anchors.bottom: bottom.bottom
            anchors.bottomMargin: status.implicitHeight+margins
            spacing: Style.spacing
            Loader {
                id: wind
                active: showWind
                asynchronous: true
                sourceComponent: Component { Wind { } }
                visible: wind.status===Loader.Ready
            }
        }
        RowLayout {
            id: bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: Style.spacing

            RowLayout {
                id: status
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft|Qt.AlignBottom
                spacing: Style.spacing
                MapInfo { }
            }
            NumbersBar {
                Layout.fillWidth: true
                settingsName: "map"
                defaults: [
                    {"bind": "est.pos.altitude", "title": "ALT", "prec": "0"},
                ]
            }
        }
    }
}
