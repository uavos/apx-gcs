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
    id: filterSettingsFact

    property var data: ({})
    property string filterType: ""
    property string filterTitle: ""
    property string detailsText: ""
    property real outputValue: 0

    flags: (Fact.Group | Fact.Section)
    title: filterTitle
    descr: detailsText

    Component.onCompleted: load(data)

    function asNumber(value, fallback)
    {
        var number = Number(value)
        return isFinite(number) ? number : fallback
    }

    function clamp(value, minValue, maxValue)
    {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function normalizedInput(inputValue)
    {
        return asNumber(inputValue, 0)
    }

    function passThrough(inputValue)
    {
        outputValue = normalizedInput(inputValue)
        return outputValue
    }

    function load(filterData)
    {
        var source = filterData && typeof filterData === "object" ? filterData : {}
        loadParameters(source)
        reset()
    }

    function save()
    {
        var filterData = saveParameters()
        if (!filterData || filterData instanceof Array || typeof filterData !== "object")
            filterData = {}

        filterData.type = filterType
        return filterData
    }

    function loadParameters(filterData)
    {
    }

    function saveParameters()
    {
        return ({})
    }

    function reset()
    {
        outputValue = 0
    }

    // Each concrete filter updates its own state and returns the latest output.
    function update(inputValue)
    {
        return passThrough(inputValue)
    }
}
