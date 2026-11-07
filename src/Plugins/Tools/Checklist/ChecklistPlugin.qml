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

import Apx.Application
import APX.Facts

/*
    Checklist plugin: aircraft check procedure.
    Steps are described in Checklist.md (plugin resources).
*/

AppPlugin {
    id: plugin

    title: qsTr("Checklist")
    descr: qsTr("Aircraft check procedures")
    icon: "clipboard-check"

    uiComponent: "main"
    unloadOnHide: false
    active: false

    ChecklistRunner {
        id: runner
    }
    // runner is reachable as ui.checklist (takeoff gating, scripts)
    Component.onCompleted: {
        ui.checklist = runner
        runner.load()
    }

    // checklist window
    property var dialog: null
    function open()
    {
        if(!dialog){
            var c = Qt.createComponent("ChecklistDialog.qml")
            if(c.status !== Component.Ready){
                console.warn("Checklist:", c.errorString())
                return
            }
            dialog = c.createObject(plugin, { "runner": runner })
        }
        if(runner.state === "idle") runner.load()
        dialog.viewed = runner.current
        dialog.open()
    }

    // menu entry
    Fact {
        id: f_checklist
        name: "checklist"
        title: qsTr("Checklist")
        descr: qsTr("Aircraft check procedure")
        icon: "clipboard-check"
        flags: Fact.Action
        section: qsTr("Checklist")
        onTriggered: plugin.open()
        Component.onCompleted: parentFact = apx.tools
    }
}
