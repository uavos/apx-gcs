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
    id: raFilter
    title: qsTr("Running average")
    descr: qsTr("Running average filter settings")
    icon: "tune"
    flags: (Fact.Group | Fact.Bool)

    property var filterType: "running_avg"
    property bool changes: false
    property var data: ({})
    property var coef: 1

    onValueChanged: fMenu.updateDescr()
    onChangesChanged: changed(changes)
    Component.onCompleted: load(data)

    signal changed(bool changesValue)

    function load(data) {
        value = data.value !== undefined ? data.value : 0
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            if(v !== undefined)
                f.value = v;
        }
        updateDescr();
        updateCoef();
    }

    function save() {
        data = {};
        data.type = filterType;
        data.value = value;
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (s === "")
                continue;
            data[settingName(f)] = s;
        }
        updateCoef();
        return data;
    }

    function settingName(f) {
        var n = f.name;
        if (n.includes("_"))
            return n.slice(0, n.indexOf("_"));
        return n;
    }
    
    function updateDescr() {
        descr = qsTr("Coef").toUpperCase() + ": K=" + raCoef.value
        changes = true; 
    }

    function updateCoef() {
        coef = raCoef.value;
        changes = false;
    }

    // Use Running Average filter
    function processValue(value, v) {
        value += (v - value) * coef;
        return value;
    }

    Fact {
        id: raCoef
        name: "coefficient"
        title: qsTr("Coefficient")
        descr: qsTr("Coefficient for filtration")
        flags: Fact.Float
        value: 1
        min: 0
        max: 1
        precision: 3
        onValueChanged: updateDescr()
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        enabled: !mChart.newItem && changes
        icon: "check-circle"
        onTriggered: sgMenu.saveSettings()
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        icon: "delete"
        onTriggered: raFilter.deleteFact();
    }
}
