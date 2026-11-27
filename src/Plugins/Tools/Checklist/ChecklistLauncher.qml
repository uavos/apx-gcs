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

import APX.Facts

/*
    Task bar button of the checklist, created by ChecklistPlugin while a procedure file exists.
    The button creates the checklist (engine and window) on press; closing the window destroys it.
*/

QtObject {
    id: launcher

    property string file       // procedure of the current unit
    property string callsign

    property Fact button: Fact {
        property bool showLauncher: true   // task bar lists the children of apx.windows with this flag (TaskBar.qml)
        name: "checklist"
        title: qsTr("Checklist")
        descr: launcher.callsign + ": " + launcher.file
        icon: "clipboard-check"
        parentFact: apx.windows
        onTriggered: {
            if(checklist.item) checklist.item.raise()
            else checklist.active = true
        }
    }

    // stage 3: the checklist exists while its window is open
    property Loader checklist: Loader {
        active: false
        sourceComponent: Checklist {
            file: launcher.file
            onClosed: checklist.active = false
        }
    }
}
