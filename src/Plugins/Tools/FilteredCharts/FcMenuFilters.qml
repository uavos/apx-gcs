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

Fact {
    id: fMenu

    property bool changes: false
    property var data: ({})

    signal removeTriggered

    onChangesChanged: { if(changes) mChart.changes = true;}

    Component.onCompleted: { 
        // updateTitle();
        // updateDescr();
        // mTitle.valueChanged.connect(updateTitle);
    }

    function load() {
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            f.value = v;
        }
        changes = false;
    }

    function save() {
        data = {};
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (f.size != 0)
                s = f.save();
            if (s === "")
                continue;
            data[settingName(f)] = s;
        }
        changes = false;
        return data;
    }

    function settingName(f) {
        var n = f.name;
        if (n.includes("_"))
            return n.slice(0, n.indexOf("_"));
        return n;
    }

    function fillData() {
        if (typeof value === 'object' && !Array.isArray(value) && value !== null) {
            data = value;
            load();
            fRunningAvg.fillData();
            changes = false;
        }
    }

    // Getting filter data
    function getRunningAvgCoef() {
        return fRunningAvg.coef;
    }

    Fact {
        id: fTypes
        name: "filters"
        title: qsTr("Filter")
        descr: qsTr("Selecting the filter to use")
        flags: Fact.Enum
        enumStrings: ["none", "running_avg"]
        onTextChanged: fMenu.value = text
        onValueChanged: changes = true // combobox index changed
    }
    FcFilterRunningAvg {
        id: fRunningAvg
        name: "running_avg"
        title: qsTr("Running average")
        descr: qsTr("Running average filter settings")
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        enabled: !mChart.newItem && changes
        icon: "check-circle"
        onTriggered: fcControl.saveSettings();
    }
}
