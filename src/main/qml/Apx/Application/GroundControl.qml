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
    The file `~/Documents/UAVOS/Plugins/main/GroundControl.qml` overrides this layout:
    either a copy of this file or a derived `GroundControl { }` item.

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
        // plugins switched off in preferences are not shown
        var pluginSettings = apx.settings.application.child("plugins")
        var p = (item.name && pluginSettings) ? pluginSettings.child(item.name) : null
        if(p && !p.value)
            return
        if(instrumentsLayout.add(item, layout, index))
            return
        if(mainLayout.add(item, layout, index))
            return
        console.error("Unsupported item layout:", layout, item)
    }

    // customization points, set them in the override file
    property bool showWindowsButton: true   // windows menu button in the task bar
    property bool showFleetControls: true   // fleet and nodes buttons in the commands panel
    property bool showControlsMenu: true    // controls group and choke in the commands menu

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

    ColumnLayout {
        anchors.fill: parent
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
