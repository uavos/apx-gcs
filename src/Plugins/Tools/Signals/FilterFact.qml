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
    id: filterFact

    property var data: ({})
    property var filterRegistry: FilterRegistry {}
    property bool isFilterItem: true

    property bool initialized: false
    property real outputValue: 0
    property real stateValue: 0
    property real covariance: 0.1
    property bool loading: false

    flags: (Fact.Group | Fact.Bool)
    icon: "tune"
    value: true

    signal removeTriggered()

    Component.onCompleted: {
        load(data)
        updateTitle()
        updateDescr()
    }

    function itemEditor()
    {
        for (var parent = filterFact.parentFact; parent; parent = parent.parentFact) {
            if (typeof parent.saveAll === "function" && parent.newItem !== undefined)
                return parent
        }

        return null
    }

    function canSaveAll()
    {
        var owner = itemEditor()
        return owner && !owner.newItem
    }

    function saveAll()
    {
        var owner = itemEditor()
        if (owner)
            owner.saveAll()
    }

    function asNumber(value, fallback)
    {
        var number = Number(value)
        return isFinite(number) ? number : fallback
    }

    function clamp(value, minValue, maxValue)
    {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function enabledValue()
    {
        return value ? true : false
    }

    function typeValue()
    {
        var text = String(typeFact.value === undefined || typeFact.value === null
                          ? typeFact.text
                          : typeFact.value).trim()
        var info = filterRegistry.typeInfo(text)
        return info ? info.value : filterRegistry.defaultType()
    }

    function isRunningAverage()
    {
        return typeValue() === "running_avg"
    }

    function isKalman()
    {
        return typeValue() === "kalman_smp"
    }

    function runningAvgCoef()
    {
        return clamp(asNumber(coefFact.value, 0.2), 0.0, 1.0)
    }

    function kalmanR()
    {
        return Math.max(0.0, asNumber(rFact.value, 0.1))
    }

    function kalmanQ()
    {
        return Math.max(0.0, asNumber(qFact.value, 0.001))
    }

    function load(filterData)
    {
        var filter = filterRegistry.normalizeFilter(filterData)
        if (!filter)
            filter = filterRegistry.defaultFilter(filterRegistry.defaultType())

        loading = true
        value = filter.enabled !== false
        typeFact.value = filter.type
        coefFact.value = filter.coef !== undefined ? filter.coef : 0.2
        rFact.value = filter.r !== undefined ? filter.r : 0.1
        qFact.value = filter.q !== undefined ? filter.q : 0.001
        reset()
        loading = false
        updateTitle()
        updateDescr()
    }

    function save()
    {
        var filter = {
            "type": typeValue(),
            "enabled": enabledValue()
        }

        switch (filter.type) {
        case "running_avg":
            filter.coef = runningAvgCoef()
            break
        case "kalman_smp":
            filter.r = kalmanR()
            filter.q = kalmanQ()
            break
        }

        return filterRegistry.normalizeFilter(filter)
    }

    function reset()
    {
        initialized = false
        outputValue = 0
        stateValue = 0
        covariance = 0.1
    }

    function step(inputValue)
    {
        var input = asNumber(inputValue, 0)
        var type = typeValue()

        if (!enabledValue())
            return input

        switch (type) {
        case "running_avg":
            if (!initialized) {
                outputValue = input
                initialized = true
                return outputValue
            }

            outputValue = outputValue * (1.0 - runningAvgCoef()) + input * runningAvgCoef()
            return outputValue
        case "kalman_smp":
            if (!initialized) {
                stateValue = input
                covariance = 0.1
                initialized = true
                return stateValue
            }

            covariance = covariance + kalmanQ()

            var denominator = covariance + kalmanR()
            var gain = denominator > 0 ? covariance / denominator : 0

            stateValue = stateValue + gain * (input - stateValue)
            covariance = (1.0 - gain) * covariance
            return stateValue
        default:
            return input
        }
    }

    function updateTitle()
    {
        title = filterRegistry.titleForType(typeValue())
    }

    function updateDescr()
    {
        var parts = []

        if (!enabledValue())
            parts.push(qsTr("Off"))

        switch (typeValue()) {
        case "running_avg":
            parts.push(qsTr("Coef") + ": " + Number(runningAvgCoef()).toFixed(2))
            break
        case "kalman_smp":
            parts.push(qsTr("R") + ": " + Number(kalmanR()))
            parts.push(qsTr("Q") + ": " + Number(kalmanQ()))
            break
        }

        descr = parts.join(", ")
    }

    onValueChanged: {
        if (!loading) {
            reset()
            updateDescr()
        }
    }

    Fact {
        id: typeFact
        name: "type"
        title: qsTr("Type")
        descr: qsTr("Filter algorithm")
        flags: Fact.Enum
        enumStrings: filterRegistry.typeValues()
        value: filterRegistry.defaultType()
        onValueChanged: {
            if (!filterFact.loading) {
                filterFact.reset()
                filterFact.updateTitle()
                filterFact.updateDescr()
            }
        }
    }

    Fact {
        id: coefFact
        name: "coef"
        title: qsTr("Coefficient")
        descr: qsTr("Running average blend coefficient")
        flags: Fact.Float
        visible: filterFact.isRunningAverage()
        value: 0.2
        onValueChanged: {
            if (!filterFact.loading) {
                filterFact.reset()
                filterFact.updateDescr()
            }
        }
    }

    Fact {
        id: rFact
        name: "r"
        title: qsTr("R")
        descr: qsTr("Measurement noise")
        flags: Fact.Float
        visible: filterFact.isKalman()
        value: 0.1
        onValueChanged: {
            if (!filterFact.loading) {
                filterFact.reset()
                filterFact.updateDescr()
            }
        }
    }

    Fact {
        id: qFact
        name: "q"
        title: qsTr("Q")
        descr: qsTr("Process noise")
        flags: Fact.Float
        visible: filterFact.isKalman()
        value: 0.001
        onValueChanged: {
            if (!filterFact.loading) {
                filterFact.reset()
                filterFact.updateDescr()
            }
        }
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        descr: qsTr("Save chart changes")
        visible: filterFact.canSaveAll()
        icon: "check-circle"
        onTriggered: filterFact.saveAll()
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove filter")
        descr: qsTr("Delete this filter from the stack")
        icon: "delete"
        onTriggered: {
            filterFact.removeTriggered()
            filterFact.deleteFact()
        }
    }
}
