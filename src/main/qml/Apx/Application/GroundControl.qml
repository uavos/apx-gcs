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

    // interface elements hidden by the override file, by element name
    property var hiddenItems: []
    function itemVisible(name)
    {
        return hiddenItems.indexOf(name) < 0
    }

    // height of the instruments panel, fraction of the window width
    property real instrumentsScale: 0.2

    // widths of the instruments panels by name, fractions of the window width:
    // "status", "pfd", "numbers", "commands"; the rest of the row is left
    // for the instruments plugins (terminal). A panel not listed here keeps
    // its natural width
    property var panelSizes: ({})

    // do not let the user resize panels
    property bool layoutLocked: false

    // sizes adjusted by the user in the layout edit mode,
    // override the values above, saved to `Preferences/layout.json`
    property var userLayout: ({})
    readonly property string layoutFile: "layout.json"

    property bool layoutEdit: false
    readonly property bool layoutEditing: layoutEdit && !layoutLocked && !maximized

    readonly property real instrumentsScaleMin: 0.08
    readonly property real instrumentsScaleMax: 0.6

    function panelSize(name)
    {
        var u = (!layoutLocked && userLayout.panels) ? userLayout.panels[name] : 0
        var v = u>0 ? u : panelSizes[name]
        return v>0 ? Math.min(v, 0.8) : 0
    }
    // size<=0 returns the panel to its default width
    function setPanelSize(name, size)
    {
        var u = JSON.parse(JSON.stringify(userLayout))
        if(!u.panels) u.panels = {}
        if(size>0) u.panels[name] = Math.round(size*10000)/10000
        else delete u.panels[name]
        userLayout = u
        layoutSaveTimer.restart()
    }
    function setInstrumentsSize(size)
    {
        var u = JSON.parse(JSON.stringify(userLayout))
        if(size>0) u.instruments = Math.round(apx.limit(size, instrumentsScaleMin, instrumentsScaleMax)*10000)/10000
        else delete u.instruments
        userLayout = u
        layoutSaveTimer.restart()
    }
    function resetLayout()
    {
        userLayout = {}
        layoutSaveTimer.restart()
    }
    function loadLayout()
    {
        var s = application.prefs.loadFile(layoutFile)
        if(!s)
            return
        try {
            var u = JSON.parse(s)
            if(u && typeof(u)==='object' && !Array.isArray(u))
                userLayout = u
        } catch(e) {
            console.warn("Can't parse "+layoutFile+": "+e)
        }
    }
    function saveLayout()
    {
        layoutSaveTimer.stop()
        var u = userLayout
        var empty = !(u.instruments>0) && !(u.panels && Object.keys(u.panels).length>0)
        if(empty) application.prefs.removeFile(layoutFile)
        else application.prefs.saveFile(layoutFile, JSON.stringify(u, null, 2))
    }
    Timer {
        id: layoutSaveTimer
        interval: 500
        onTriggered: saveLayout()
    }
    Component.onCompleted: loadLayout()
    Component.onDestruction: if(layoutSaveTimer.running) saveLayout()

    //internal
    readonly property real instrumentsFraction: {
        var u = layoutLocked ? 0 : userLayout.instruments
        return apx.limit(u>0 ? u : instrumentsScale, instrumentsScaleMin, instrumentsScaleMax)
    }
    // the main layout below never gets less than a half of the window
    readonly property int instrumentsHeight: Math.min(width*instrumentsFraction, height/2)
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
        Rectangle {
            visible: instrumentsLayout.visible
            Layout.fillWidth: true
            implicitHeight: visible?1:0
            border.width: 0
            color: sepColor
            z: 10
            LayoutHandle {
                vertical: true
                active: layoutEditing
                property real size0: 0
                onStarted: size0=instrumentsLayout.height
                onMoved: (delta) => {
                    // keep the stored value within what is actually shown
                    var h=Math.min(size0+delta, groundControl.height/2)
                    setInstrumentsSize(h/groundControl.width)
                }
                onReset: setInstrumentsSize(0)
            }
        }

        MainLayout {
            id: mainLayout
            state: groundControl.state
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: instrumentsHeight
        }
    }
}
