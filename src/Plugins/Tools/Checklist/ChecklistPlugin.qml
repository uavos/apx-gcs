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
import QtCore
import Qt.labs.folderlistmodel

import Apx.Application

/*
    Checklist plugin: aircraft check procedure driven by a Markdown file.

    Procedures live in <Documents>/UAVOS/Checklists/*.md. The first file (by name) whose name is
    the beginning of the current unit callsign (case insensitive: Borey-123 -> borey.md) is the
    procedure of that unit; no file, no checklist.

    Loaded in stages: this file only looks the file up (stage 1); while one exists
    ChecklistLauncher, the task bar button, is loaded (stage 2); the button loads Checklist,
    the engine with its window, on press (stage 3). Nothing else exists until it is needed.

    The check is started on the ground only: the button is shown and Start works while the
    unit is in TAXI, not in the air and the uplink is enabled (`blocked` names the reason
    otherwise). A running check is aborted when the unit gets airborne; mode changes during
    the run are the procedure's own business. With `checklistAuto` set by the main layout
    (the lite profile) the checklist opens itself when the unit enters TAXI.
*/

AppPlugin {
    id: plugin

    title: qsTr("Checklist")
    descr: qsTr("Aircraft check procedures")
    icon: "clipboard-check"

    // ---------------------------------------------------------------- launch conditions
    readonly property var f_mode: mandala.cmd.proc.mode
    readonly property var f_inair: mandala.cmd.ins.inair
    readonly property var f_sysmode: mandala.est.sys.mode
    readonly property bool airborne: f_inair.value == ins_inair_yes || f_sysmode.text === "airborne"
    // why the check cannot be started now, empty when it can
    readonly property string blocked: airborne ? qsTr("The aircraft is in the air")
                                    : f_mode.value != proc_mode_TAXI ? qsTr("Switch the aircraft to TAXI")
                                    : apx.datalink.readonly.value ? qsTr("Uplink is disabled")
                                    : ""

    // main layout flag (lite profile): open the checklist when the check becomes possible
    uiComponent: "main"
    property bool autoOpen: false
    onConfigure: autoOpen = ui.main.checklistAuto === true

    // ---------------------------------------------------------------- procedure file
    readonly property string dir: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/UAVOS/Checklists"
    readonly property string callsign: apx.fleet.current ? apx.fleet.current.title : ""
    property string file: ""         // procedure of the current unit, empty when there is none

    FolderListModel {
        id: folder
        folder: plugin.dir
        nameFilters: ["*.md"]
        caseSensitive: false
        sortField: FolderListModel.Name
        sortCaseSensitive: false
        showDirs: false
        onCountChanged: plugin.match()
        onStatusChanged: if(status === FolderListModel.Ready) plugin.match()
    }
    onCallsignChanged: match()
    // the first file (by name) that starts the callsign
    function match()
    {
        var cs = callsign.toLowerCase()
        var found = ""
        for (var i = 0; i < folder.count && !found; ++i) {
            var n = String(folder.get(i, "fileBaseName")).toLowerCase()
            if(n && cs && cs.indexOf(n) === 0) found = String(folder.get(i, "filePath"))
        }
        file = found
    }

    // stage 2: the task bar button exists while there is a procedure file
    Loader {
        active: plugin.file !== ""
        sourceComponent: ChecklistLauncher {
            file: plugin.file
            callsign: plugin.callsign
            blocked: plugin.blocked
            airborne: plugin.airborne
            autoOpen: plugin.autoOpen
        }
    }
}
