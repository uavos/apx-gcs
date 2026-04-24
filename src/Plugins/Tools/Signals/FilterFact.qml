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
    property bool loading: false
    property string filterType: filterRegistry.defaultType()

    flags: (Fact.Group | Fact.Bool)
    icon: "tune"
    value: true
    title: filterRegistry.titleForType(filterType)
    descr: filterDescription()

    signal removeTriggered()

    readonly property bool filterEnabled: value !== false
    readonly property var currentFilterFact: filterBody.size > 0 ? filterBody.child(0) : null

    Component.onCompleted: load(data)

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

    function createFact(parent, url, opts)
    {
        var component = Qt.createComponent(Qt.resolvedUrl(url))
        if (component.status === Component.Ready) {
            var properties = opts || {}
            properties.parentFact = parent
            return component.createObject(parent, properties)
        }

        console.log(component.errorString())
        return null
    }

    function selectedTypeText()
    {
        if (typeFact.text !== undefined && typeFact.text !== null)
            return String(typeFact.text).trim()

        if (typeFact.value !== undefined && typeFact.value !== null)
            return String(typeFact.value).trim()

        return ""
    }

    function filterDescription()
    {
        var details = currentFilterFact && currentFilterFact.descr !== undefined ? String(currentFilterFact.descr) : ""
        if (filterEnabled)
            return details

        return details !== "" ? qsTr("Off") + ", " + details : qsTr("Off")
    }

    function createFilterBody(filterData)
    {
        var info = filterRegistry.typeInfo(filterType)
        if (!info)
            return null

        filterBody.deleteChildren()
        return createFact(filterBody, info.source, {
                              "data": filterData
                          })
    }

    function setFilterType(type, filterData)
    {
        var normalizedType = filterRegistry.valueForType(type)
        var normalizedData = filterRegistry.normalizeFilter(filterData)

        filterType = normalizedType
        createFilterBody(normalizedData ? normalizedData : filterRegistry.defaultFilter(normalizedType))
    }

    function load(filterData)
    {
        var filter = filterRegistry.normalizeFilter(filterData)
        if (!filter)
            filter = filterRegistry.defaultFilter(filterRegistry.defaultType())

        loading = true
        value = filter.enabled !== false
        filterType = filter.type
        typeFact.value = filterRegistry.titleForType(filter.type)
        createFilterBody(filter)
        reset()
        loading = false
    }

    function save()
    {
        var filter = currentFilterFact && typeof currentFilterFact.save === "function"
                     ? currentFilterFact.save()
                     : filterRegistry.defaultFilter(filterType)
        if (!filter)
            return null

        filter.enabled = filterEnabled
        filter.type = filterType
        return filterRegistry.normalizeFilter(filter)
    }

    function reset()
    {
        if (currentFilterFact && typeof currentFilterFact.reset === "function")
            currentFilterFact.reset()
    }

    // The wrapper controls enable/type while the loaded child owns the actual math.
    function update(inputValue)
    {
        var input = Number(inputValue)
        if (!isFinite(input))
            input = 0

        if (!filterEnabled)
            return input

        if (currentFilterFact && typeof currentFilterFact.update === "function")
            return currentFilterFact.update(input)

        return input
    }

    onValueChanged: {
        if (!loading)
            reset()
    }

    Fact {
        id: typeFact
        name: "type"
        title: qsTr("Type")
        descr: qsTr("Filter algorithm")
        flags: Fact.Enum
        enumStrings: filterRegistry.typeTitles()
        value: filterRegistry.titleForType(filterRegistry.defaultType())
        onValueChanged: {
            if (filterFact.loading)
                return

            var selectedType = filterRegistry.valueForType(filterFact.selectedTypeText())
            if (selectedType === filterFact.filterType)
                return

            filterFact.setFilterType(selectedType, filterRegistry.defaultFilter(selectedType))
            filterFact.reset()
        }
    }

    Fact {
        id: filterBody
        title: qsTr("Settings")
        descr: qsTr("Parameters for the selected filter type")
        flags: (Fact.Group | Fact.Section)
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
