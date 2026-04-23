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
    property real q: 1
    property real r: 1
    property var data: ({})

    // Kalman state
    property real kState: 0
    property real kCovariance: 0.1
    property bool initialized: false

    function filterValue(input) {
        if (!initialized) {
            kState = input;
            kCovariance = 0.1;
            initialized = true;
        }
        // Time update - prediction
        var x0 = kState;
        var p0 = kCovariance + q;

        // Measurement update - correction
        var k = p0 / (p0 + r);
        kState = x0 + k * (input - x0);
        kCovariance = (1 - k) * p0;
        return kState;
    }

    function resetState() {
        initialized = false;
    }

    function loadFromObject(obj) {
        data = obj || {};
        ksMeasNoise.value = data.r !== undefined ? data.r : (data.measurement_noise !== undefined ? data.measurement_noise : 1);
        ksEnvNoise.value = data.q !== undefined ? data.q : (data.environment_noise !== undefined ? data.environment_noise : 1);
        updateCoefs();
    }

    function updateFilterValue() {
        ksFilter.value = "Km=" + ksMeasNoise.value + ",Ke=" + ksEnvNoise.value;
        changes = true;
    }

    function updateCoefs() {
        r = ksMeasNoise.value;
        q = ksEnvNoise.value;
        changes = false;
    }

    function save() {
        updateCoefs();
        data = {
            r: ksMeasNoise.value,
            q: ksEnvNoise.value
        };
        return data;
    }

    Fact {
        id: ksMeasNoise
        name: "r"
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
        name: "q"
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

