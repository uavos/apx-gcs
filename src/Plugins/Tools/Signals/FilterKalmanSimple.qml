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
    id: ksFilter

    flags: Fact.Group

    property bool changes: false
    property var coefs: [1, 1]
    property var data: ({})

    // Kalman state
    property real kState: 0
    property real kCovariance: 0.1
    property bool initialized: false

    onChangesChanged: { if (changes) menuFilters.changes = true; }

    function filterValue(input) {
        if (!initialized) {
            kState = input;
            kCovariance = 0.1;
            initialized = true;
        }
        // Time update - prediction
        var x0 = kState;
        var p0 = kCovariance + coefs[0];

        // Measurement update - correction
        var k = p0 / (p0 + coefs[1]);
        kState = x0 + k * (input - x0);
        kCovariance = (1 - k) * p0;
        return kState;
    }

    function resetState() {
        initialized = false;
    }

    function load() {
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            if (v !== undefined)
                f.value = v;
        }
        updateCoefs();
    }

    function save() {
        data = {};
        for (var i = 0; i < size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (s === "")
                continue;
            data[settingName(f)] = s;
        }
        updateCoefs();
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

    function updateFilterValue() {
        ksFilter.value = "Km=" + ksMeasNoise.value + ",Ke=" + ksEnvNoise.value;
        changes = true;
    }

    function updateCoefs() {
        coefs = [ksMeasNoise.value, ksEnvNoise.value];
        changes = false;
    }

    Fact {
        id: ksMeasNoise
        name: "measurement_noise"
        title: qsTr("Measurement noise")
        descr: qsTr("Coefficient of measurement noise")
        flags: Fact.Float
        value: 1
        min: 0
        max: 10000
        precision: 3
        onValueChanged: updateFilterValue()
    }
    Fact {
        id: ksEnvNoise
        name: "environment_noise"
        title: qsTr("Environment noise")
        descr: qsTr("Coefficient of environment noise")
        flags: Fact.Float
        value: 1
        min: 0
        max: 10000
        precision: 3
        onValueChanged: updateFilterValue()
    }
}

