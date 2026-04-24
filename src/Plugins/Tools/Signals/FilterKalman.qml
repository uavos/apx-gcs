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

FilterBase {
    id: filterFact

    property real defaultR: 0.1
    property real defaultQ: 0.001
    property bool initialized: false
    property real stateEstimate: 0
    property real covariance: 0.1
    readonly property real measurementNoise: Math.max(0.0, asNumber(rFact.value, defaultR))
    readonly property real processNoise: Math.max(0.0, asNumber(qFact.value, defaultQ))

    filterType: "kalman_smp"
    filterTitle: qsTr("Simple Kalman")
    detailsText: qsTr("R") + ": " + Number(measurementNoise)
                 + ", " + qsTr("Q") + ": " + Number(processNoise)

    function loadParameters(filterData)
    {
        rFact.value = Math.max(0.0, asNumber(filterData.r, defaultR))
        qFact.value = Math.max(0.0, asNumber(filterData.q, defaultQ))
    }

    function saveParameters()
    {
        return {
            "r": measurementNoise,
            "q": processNoise
        }
    }

    function reset()
    {
        initialized = false
        stateEstimate = 0
        covariance = 0.1
        outputValue = 0
    }

    function update(inputValue)
    {
        var input = normalizedInput(inputValue)
        if (!filterEnabled)
            return passThrough(input)

        if (!initialized) {
            stateEstimate = input
            covariance = 0.1
            initialized = true
        } else {
            covariance = covariance + processNoise

            var denominator = covariance + measurementNoise
            var gain = denominator > 0 ? covariance / denominator : 0

            stateEstimate = stateEstimate + gain * (input - stateEstimate)
            covariance = (1.0 - gain) * covariance
        }

        outputValue = stateEstimate
        return outputValue
    }

    Fact {
        id: rFact
        name: "r"
        title: qsTr("R")
        descr: qsTr("Measurement noise")
        flags: Fact.Float
        value: defaultR
    }

    Fact {
        id: qFact
        name: "q"
        title: qsTr("Q")
        descr: qsTr("Process noise")
        flags: Fact.Float
        value: defaultQ
    }
}
