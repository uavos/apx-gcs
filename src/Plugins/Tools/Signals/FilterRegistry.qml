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
import QtQml

QtObject {
    id: registry

    readonly property string genericFilterSource: Qt.resolvedUrl("FilterFact.qml")

    readonly property var typeOptions: [
        {
            "title": qsTr("Running average"),
            "value": "running_avg",
            "source": genericFilterSource
        },
        {
            "title": qsTr("Simple Kalman"),
            "value": "kalman_smp",
            "source": genericFilterSource
        }
    ]

    function cloneValue(value)
    {
        if (value === undefined || value === null)
            return value

        return JSON.parse(JSON.stringify(value))
    }

    function asObject(value)
    {
        if (!value || value instanceof Array || typeof value !== "object")
            return {}

        return value
    }

    function asString(value, fallback)
    {
        if (fallback === undefined)
            fallback = ""

        if (value === undefined || value === null)
            return fallback

        return String(value)
    }

    function asBool(value, fallback)
    {
        if (value === undefined || value === null)
            return fallback

        return !!value
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

    function typeInfo(type)
    {
        for (var i = 0; i < typeOptions.length; ++i) {
            if (typeOptions[i].value === type)
                return typeOptions[i]
        }

        return null
    }

    function typeIndex(type)
    {
        for (var i = 0; i < typeOptions.length; ++i) {
            if (typeOptions[i].value === type)
                return i
        }

        return 0
    }

    function titleForType(type)
    {
        var info = typeInfo(type)
        return info ? info.title : asString(type, "")
    }

    function componentSource(type)
    {
        var info = typeInfo(type)
        return info ? info.source : ""
    }

    function defaultType()
    {
        return typeOptions.length > 0 ? typeOptions[0].value : ""
    }

    function typeValues()
    {
        var values = []

        for (var i = 0; i < typeOptions.length; ++i)
            values.push(typeOptions[i].value)

        return values
    }

    function defaultFilter(type)
    {
        switch (type) {
        case "kalman_smp":
            return {
                "type": "kalman_smp",
                "enabled": true,
                "r": 0.1,
                "q": 0.001
            }
        case "running_avg":
            return {
                "type": "running_avg",
                "enabled": true,
                "coef": 0.2
            }
        default:
            return null
        }
    }

    function normalizeFilter(filterData)
    {
        var source = asObject(filterData)
        var type = asString(source.type, "")
        var filter = defaultFilter(type)

        if (!filter)
            return null

        filter = cloneValue(filter)
        filter.enabled = asBool(source.enabled, true)

        switch (filter.type) {
        case "running_avg":
            filter.coef = clamp(asNumber(source.coef, filter.coef), 0.0, 1.0)
            break
        case "kalman_smp":
            filter.r = Math.max(0.0, asNumber(source.r, filter.r))
            filter.q = Math.max(0.0, asNumber(source.q, filter.q))
            break
        }

        return filter
    }

    function normalizeFilters(filtersData)
    {
        var source = filtersData instanceof Array ? filtersData : []
        var list = []

        for (var i = 0; i < source.length; ++i) {
            var filter = normalizeFilter(source[i])
            if (filter)
                list.push(filter)
        }

        return list
    }
}
