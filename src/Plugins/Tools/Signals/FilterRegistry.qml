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

    // To add a filter: create a dedicated Filter*.qml component and register it here.
    readonly property var typeOptions: [
        {
            "title": qsTr("Running average"),
            "value": "running_avg",
            "source": Qt.resolvedUrl("FilterRunningAverage.qml"),
            "defaults": {
                "type": "running_avg",
                "enabled": true,
                "coef": 0.2
            }
        },
        {
            "title": qsTr("Simple Kalman"),
            "value": "kalman_smp",
            "source": Qt.resolvedUrl("FilterKalman.qml"),
            "defaults": {
                "type": "kalman_smp",
                "enabled": true,
                "r": 0.1,
                "q": 0.001
            }
        }
    ]

    function cloneValue(value)
    {
        if (value === undefined || value === null)
            return value

        return JSON.parse(JSON.stringify(value))
    }

    function asString(value, fallback)
    {
        if (fallback === undefined)
            fallback = ""

        if (value === undefined || value === null)
            return fallback

        return String(value)
    }

    function typeInfo(type)
    {
        var text = asString(type, "")

        for (var i = 0; i < typeOptions.length; ++i) {
            if (typeOptions[i].value === text || typeOptions[i].title === text)
                return typeOptions[i]
        }

        return null
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

    function typeTitles()
    {
        var values = []

        for (var i = 0; i < typeOptions.length; ++i)
            values.push(typeOptions[i].title)

        return values
    }

    function valueForType(type)
    {
        var info = typeInfo(type)
        return info ? info.value : defaultType()
    }

    function defaultType()
    {
        return typeOptions.length > 0 ? typeOptions[0].value : ""
    }

    function defaultFilter(type)
    {
        var info = typeInfo(type)
        return info ? cloneValue(info.defaults) : null
    }

    function normalizeFilter(filterData)
    {
        if (!filterData || filterData instanceof Array || typeof filterData !== "object")
            return null

        var source = cloneValue(filterData)
        var filter = defaultFilter(source.type)

        if (!filter)
            return null

        for (var key in source)
            filter[key] = source[key]

        filter.type = defaultFilter(source.type).type
        if (filter.enabled === undefined)
            filter.enabled = true

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
