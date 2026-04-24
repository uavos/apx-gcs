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

    property real defaultCoefficient: 0.2
    property bool initialized: false
    property real averageValue: 0
    readonly property real coefficient: clamp(asNumber(coefFact.value, defaultCoefficient), 0.0, 1.0)

    filterType: "running_avg"
    filterTitle: qsTr("Running average")
    detailsText: qsTr("Coef") + ": " + Number(coefficient).toFixed(2)

    function loadParameters(filterData)
    {
        coefFact.value = clamp(asNumber(filterData.coef, defaultCoefficient), 0.0, 1.0)
    }

    function saveParameters()
    {
        return {
            "coef": coefficient
        }
    }

    function reset()
    {
        initialized = false
        averageValue = 0
        outputValue = 0
    }

    function update(inputValue)
    {
        var input = normalizedInput(inputValue)
        if (!filterEnabled)
            return passThrough(input)

        if (!initialized) {
            averageValue = input
            initialized = true
        } else {
            averageValue = averageValue * (1.0 - coefficient) + input * coefficient
        }

        outputValue = averageValue
        return outputValue
    }

    Fact {
        id: coefFact
        name: "coef"
        title: qsTr("Coefficient")
        descr: qsTr("Running average blend coefficient")
        flags: Fact.Float
        value: defaultCoefficient
    }
}
