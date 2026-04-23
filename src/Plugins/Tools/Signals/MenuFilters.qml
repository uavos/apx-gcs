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

// Filter selector + params for a single chart item.
// Exposes value as the currently selected filter type string.
// To add a new filter type:
//   1. Create FilterMyType.qml with filterValue(input)/resetState() API
//   2. Add instance below with name matching the type string
//   3. Add the type string to fTypes.enumStrings
//   4. Handle it in MenuItem.qml updateValue() filter loop
Fact {
    id: menuFilters

    property bool changes: false
    property var data: ({})

    signal removeTriggered

    onChangesChanged: { if (changes) menuItem.changes = true; }

    // Returns currently active filter type string ("none", "running_avg", "kalman_smp")
    property string filterType: fTypes.text

    function getFilterType() { return fTypes.text; }
    function getRunningAvgCoef() { return fRunningAvg.coef; }
    function getKalmanSimpleCoefs() { return fKalmanSimple.coefs; }

    // Apply all enabled filters in sequence (future: loop over filter list)
    // For current single-filter model, applies selected filter type
    function applyFilters(v, stateObj) {
        var type = fTypes.text;
        if (type === "running_avg") {
            return fRunningAvg.filterValue(v);
        } else if (type === "kalman_smp") {
            return fKalmanSimple.filterValue(v);
        }
        return v;
    }

    function resetFilterState() {
        fRunningAvg.resetState();
        fKalmanSimple.resetState();
    }

    function load() {
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            if (v !== undefined)
                f.value = v;
        }
        fRunningAvg.fillData();
        fKalmanSimple.fillData();
        changes = false;
    }

    function save() {
        data = {};
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (f.size !== 0)
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
        }
    }

    Fact {
        id: fTypes
        name: "filters"
        title: qsTr("Filter")
        descr: qsTr("Selecting the filter to use")
        flags: Fact.Enum
        enumStrings: ["none", "running_avg", "kalman_smp"]
        onTextChanged: menuFilters.value = text
        onValueChanged: changes = true
    }
    FilterRunningAvg {
        id: fRunningAvg
        name: "running_avg"
        title: qsTr("Running average")
        descr: qsTr("Running average filter settings")
    }
    FilterKalmanSimple {
        id: fKalmanSimple
        name: "kalman_smp"
        title: qsTr("Kalman simple")
        descr: qsTr("Simple kalman filter settings")
    }

}

