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
import QtCore

import "qrc:/Apx/Application"

/*

    Main layout QML.
    Copy this file to `~/Documents/UAVOS/Plugins` and edit to override.

*/


Item {
    id: groundControl


    enum Layout {
        Instrument,
        Main,
        Notifications
    }
    function add(item, layout, index)
    {
        if(instrumentsLayout.add(item, layout, index))
            return
        if(mainLayout.add(item, layout, index))
            return
        console.error("Unsupported item layout:", layout, item)
    }

    //internal
    readonly property int instrumentsHeight: width*0.2
    readonly property color sepColor: "#244"

    Settings {
        id: settings
        category: "Layout"
        property alias state: groundControl.state
    }

    state: "normal"
    states: [
        State {
            name: "normal"
            PropertyChanges {
                target: instrumentsLayout;
                visible: true
            }
        },
        State {
            name: "maximized"
            PropertyChanges {
                target: instrumentsLayout;
                visible: false
            }
        }
    ]

    property bool maximized: state=="maximized"
    function toggleState()
    {
        state=maximized?"normal":"maximized"
    }

    GridLayout {
        anchors.fill: parent
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            InstrumentsLayout {
                id: instrumentsLayout
                state: groundControl.state
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.preferredHeight: instrumentsHeight
            }
            Rectangle { visible: instrumentsLayout.visible; Layout.fillWidth: true; implicitHeight: visible?1:0; border.width: 0; color: sepColor; }

            MainLayout {
                id: mainLayout
                state: groundControl.state
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: instrumentsHeight
            }
        }
    }
}
