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
    title: qsTr("Kalman simple")
    descr: qsTr("Simple kalman filter settings")
    icon: "tune"
    flags: (Fact.Group | Fact.Bool)

    property var filterType: "kalman_smp"
    property bool changes: false
    property var measCoef: 1
    property var envCoef: 1
    property var data: ({})

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
        updateDescr()
        updateCoefs();
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
        updateCoefs();
        return data;
    }

    function settingName(f) {
        var n = f.name;
        if (n.includes("_"))
            return n.slice(0, n.indexOf("_"));
        return n;
    }

    function updateDescr() {
        descr = qsTr("COEF") + ": " + "Km=" + ksMeasNoise.value + ", Ke=" + ksEnvNoise.value;
        changes = true; 
    }

    function updateCoefs() {
        measCoef = ksMeasNoise.value;
        envCoef = ksEnvNoise.value;
        changes = false;
    }

    // Use Kalman Simple filter
    property var state: 0
    property var covariance: 0.1

    function setKalmanState(st, cv) {
        state = st;
        covariance = cv;
    }

    function processValue(value, v) {
        // Time update - prediction
        var x0 = state;
        var p0 = covariance + measCoef;

        // Measurement update - correction
        var k = p0 / (p0 + envCoef);
        state = x0 + k * (v - x0);
        covariance = (1 - k) * p0;
        value = state;
        return value;
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
        onValueChanged: updateDescr()
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
        onTriggered: ksFilter.deleteFact();
    }
}
