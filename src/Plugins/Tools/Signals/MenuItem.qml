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
import QtQuick.Controls.Material

import APX.Facts

Fact {
    id: itemFact

    property bool newItem: false
    property var data: ({})
    property var signalsModel: null
    property var setFact: null
    property var pageFact: null

    property string colorValue: ""
    property var filtersData: []
    property string saveError: ""
    property var filterRegistry: FilterRegistry {}
    readonly property var colorBaseLabels: [
        qsTr("Red"),
        qsTr("Pink"),
        qsTr("Purple"),
        qsTr("Deep Purple"),
        qsTr("Indigo"),
        qsTr("Blue"),
        qsTr("Light Blue"),
        qsTr("Cyan"),
        qsTr("Teal"),
        qsTr("Green"),
        qsTr("Orange"),
        qsTr("Blue Grey")
    ]
    readonly property var colorBaseValues: [
        Material.Red,
        Material.Pink,
        Material.Purple,
        Material.DeepPurple,
        Material.Indigo,
        Material.Blue,
        Material.LightBlue,
        Material.Cyan,
        Material.Teal,
        Material.Green,
        Material.Orange,
        Material.BlueGrey
    ]
    readonly property var colorShadeLabels: [
        qsTr("300"),
        qsTr("500"),
        qsTr("700"),
        qsTr("900")
    ]
    readonly property var colorShadeValues: [
        Material.Shade300,
        Material.Shade500,
        Material.Shade700,
        Material.Shade900
    ]

    flags: Fact.Group
    title: newItem ? qsTr("Add new item") : bindText()
    descr: itemDescription()

    signal addTriggered()
    signal removeTriggered()

    Component.onCompleted: {
        load()
        rebuildColorChoices()
    }

    function rootEditor()
    {
        for (var parent = itemFact.parentFact; parent; parent = parent.parentFact) {
            if (typeof parent.saveSettings === "function")
                return parent
        }

        return null
    }

    function saveAll()
    {
        var root = rootEditor()
        if (root)
            root.saveSettings()
    }

    function createFact(parent, url, opts)
    {
        var component = Qt.createComponent(Qt.resolvedUrl(url))
        if (component.status === Component.Ready) {
            var properties = opts || {}
            properties.parentFact = parent
            var child = component.createObject(parent, properties)
            return child
        }

        console.log(component.errorString())
        return null
    }

    function cloneValue(value)
    {
        if (value === undefined || value === null)
            return value
        return JSON.parse(JSON.stringify(value))
    }

    function normalizeColorValue(value)
    {
        var text = String(value === undefined || value === null ? "" : value).trim()
        if (text === "")
            return ""
        return text.toUpperCase()
    }

    function normalizeBindValue(value)
    {
        var text = String(value === undefined || value === null ? "" : value).trim()
        var simplePath = /^(?:mandala\.)?[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+(?:\.value)?$/

        if (text === "" || !simplePath.test(text))
            return text

        if (text.startsWith("mandala."))
            text = text.slice(8)
        if (text.endsWith(".value"))
            text = text.slice(0, -6)

        return text
    }

    function factTextValue(factObject)
    {
        if (!factObject)
            return ""

        if (factObject.text !== undefined && factObject.text !== null)
            return String(factObject.text).trim()

        if (factObject.value !== undefined && factObject.value !== null)
            return String(factObject.value).trim()

        return ""
    }

    function bindText()
    {
        return normalizeBindValue(factTextValue(mBind))
    }

    function saveTarget()
    {
        return normalizeBindValue(factTextValue(mSaveFact))
    }

    function colorValueCurrent()
    {
        return colorValue
    }

    function warningText()
    {
        return factTextValue(mWarning)
    }

    function colorSummary()
    {
        return colorValue !== "" ? colorValue : qsTr("Auto")
    }

    function filterSummary(filterData)
    {
        var filter = filterData || {}
        var label = filterRegistry.titleForType(filter.type)
        if (filter.enabled === false)
            label += " (" + qsTr("off") + ")"
        return label
    }

    function liveFilterSummary(filterFact)
    {
        if (!filterFact)
            return ""

        return filterFact.descr !== "" ? filterFact.title + ": " + filterFact.descr : filterFact.title
    }

    function clearFilterFacts()
    {
        if (!mFiltersGroup)
            return

        for (var i = mFiltersGroup.size - 1; i >= 0; --i) {
            var child = mFiltersGroup.child(i)
            if (child && child.isFilterItem)
                child.deleteFact()
        }
    }

    function createFilterFact(filterData)
    {
        var child = createFact(mFiltersGroup, "FilterFact.qml", {
                                   "data": filterData,
                                   "filterRegistry": itemFact.filterRegistry
                               })
        if (!child)
            return null

        return child
    }

    function exportFilters()
    {
        var list = []

        var liveFilters = filterFacts()
        for (var i = 0; i < liveFilters.length; ++i) {
            var child = liveFilters[i]
            if (!child || typeof child.save !== "function")
                continue

            var filterData = child.save()
            if (filterData)
                list.push(filterRegistry.normalizeFilter(filterData))
        }

        return list
    }

    function filterFacts()
    {
        var list = []

        if (!mFiltersGroup)
            return list

        for (var i = 0; i < mFiltersGroup.size; ++i) {
            var child = mFiltersGroup.child(i)
            if (child && child.isFilterItem)
                list.push(child)
        }

        return list
    }

    function currentFilters()
    {
        var live = exportFilters()
        if (live.length > 0)
            return live
        return filterRegistry.normalizeFilters(cloneValue(filtersData))
    }

    function rebuildFilters()
    {
        clearFilterFacts()

        var source = filterRegistry.normalizeFilters(cloneValue(filtersData))
        for (var i = 0; i < source.length; ++i)
            createFilterFact(source[i])
    }

    function filtersSummary()
    {
        var liveFilters = filterFacts()
        if (liveFilters.length > 0) {
            var liveParts = []
            for (var i = 0; i < liveFilters.length; ++i)
                liveParts.push(liveFilterSummary(liveFilters[i]))
            return liveParts.join(", ")
        }

        var list = currentFilters()
        if (!(list instanceof Array) || list.length <= 0)
            return qsTr("None")

        var parts = []
        for (var i = 0; i < list.length; ++i)
            parts.push(filterSummary(list[i]))
        return parts.join(", ")
    }

    // The item owns the live filter chain so the chart only provides a raw sample.
    function updateFilters(inputValue)
    {
        var output = Number(inputValue)
        if (!isFinite(output))
            output = 0

        var liveFilters = filterFacts()
        for (var i = 0; i < liveFilters.length; ++i) {
            var filterFact = liveFilters[i]
            if (!filterFact || typeof filterFact.update !== "function")
                continue

            output = filterFact.update(output)
        }

        return output
    }

    function setColorValue(value)
    {
        colorValue = normalizeColorValue(value)
    }

    function clearColorChoices()
    {
        if (!mColorPage)
            return

        for (var i = mColorPage.size - 1; i >= 0; --i) {
            var child = mColorPage.child(i)
            if (child && child.isColorChoice)
                child.deleteFact()
        }
    }

    function rebuildColorChoices()
    {
        clearColorChoices()

        createFact(mColorPage, "ColorChoiceFact.qml", {
                       "itemOwner": itemFact,
                       "title": qsTr("Auto"),
                       "descr": qsTr("Use automatic series color"),
                       "colorValue": "",
                       "section": "",
                       "value": qsTr("Auto")
                   })

        for (var shadeIndex = 0; shadeIndex < colorShadeValues.length; ++shadeIndex) {
            var shadeLabel = colorShadeLabels[shadeIndex]
            for (var colorIndex = 0; colorIndex < colorBaseValues.length; ++colorIndex) {
                var colorCode = Material.color(colorBaseValues[colorIndex],
                                               colorShadeValues[shadeIndex]).toString().toUpperCase()
                createFact(mColorPage, "ColorChoiceFact.qml", {
                               "itemOwner": itemFact,
                               "title": colorBaseLabels[colorIndex],
                               "descr": colorCode,
                               "colorValue": colorCode,
                               "section": shadeLabel,
                               "value": colorCode
                           })
            }
        }
    }

    function setFiltersData(value)
    {
        filtersData = filterRegistry.normalizeFilters(cloneValue(value))
        rebuildFilters()
    }

    function addFilter()
    {
        return createFilterFact(filterRegistry.defaultFilter(filterRegistry.defaultType()))
    }

    function validationError()
    {
        var target = saveTarget()
        if (target === "")
            return ""

        if (!target.startsWith("sns.scr."))
            return qsTr("Save target must start with sns.scr.")

        if (itemFact.pageFact && typeof itemFact.pageFact.isSaveTargetUsedLocal === "function"
                && itemFact.pageFact.isSaveTargetUsedLocal(target, itemFact))
            return qsTr("Save target is already used in this page.")

        if (itemFact.setFact && typeof itemFact.setFact.isSaveTargetUsed === "function"
                && itemFact.setFact.isSaveTargetUsed(target, itemFact))
            return qsTr("Save target is already used in this set.")

        return ""
    }

    function refreshValidation()
    {
        saveError = validationError()
    }

    function canSave()
    {
        return bindText() !== "" && validationError() === ""
    }

    function load()
    {
        mBind.value = normalizeBindValue(data && data.bind !== undefined ? data.bind : "")
        colorValue = normalizeColorValue(data ? data.color : "")
        filtersData = filterRegistry.normalizeFilters(cloneValue(data && data.filters instanceof Array
                                                                 ? data.filters
                                                                 : []))
        rebuildFilters()
        mWarning.value = data && data.warning !== undefined ? data.warning : ""
        mSaveFact.value = normalizeBindValue(data && data.save !== undefined ? data.save : null)
        refreshValidation()
    }

    function save()
    {
        if (!canSave())
            return null

        var item = {
            "bind": bindText(),
            "filters": exportFilters()
        }

        if (colorValue !== "")
            item.color = colorValue
        if (factTextValue(mWarning) !== "")
            item.warning = factTextValue(mWarning)
        if (saveTarget() !== "")
            item.save = saveTarget()

        return item
    }

    function itemDescription()
    {
        var details = []
        if (colorValue !== "")
            details.push(qsTr("Color") + ": " + colorValue)
        if (filtersSummary() !== qsTr("None"))
            details.push(qsTr("Filters") + ": " + filtersSummary())
        if (factTextValue(mWarning) !== "")
            details.push(qsTr("Warning") + ": " + factTextValue(mWarning))
        if (saveTarget() !== "")
            details.push(qsTr("Save") + ": " + saveTarget())
        if (saveError !== "")
            details.push(saveError)

        return details.join(", ")
    }

    Fact {
        id: mFact
        title: qsTr("Binding")
        descr: qsTr("Pick a mandala fact")
        flags: Fact.Int
        units: "mandala"
        onTextChanged: {
            if (value)
                mBind.setValue(itemFact.normalizeBindValue(text))
        }
    }

    Fact {
        id: mBind
        name: "bind"
        title: qsTr("Expression")
        descr: qsTr("Mandala path or JavaScript expression")
        flags: Fact.Text
    }

    Fact {
        id: mColorPage
        title: qsTr("Color")
        descr: qsTr("Series color override")
        flags: Fact.Group
        value: itemFact.colorSummary()
        opts: ({
                   "editor": Qt.resolvedUrl("ColorChooser.qml")
               })
        property var itemOwner: itemFact
    }

    Fact {
        id: mFiltersGroup
        title: qsTr("Filters")
        descr: itemFact.filtersSummary()
        // icon: "tune"
        flags: (Fact.Group | Fact.DragChildren)

        Fact {
            flags: Fact.Action
            title: qsTr("Add filter")
            descr: qsTr("Append a new filter to the stack")
            icon: "plus-circle"
            onTriggered: itemFact.addFilter()
        }

        Fact {
            flags: (Fact.Action | Fact.Apply)
            title: qsTr("Save")
            descr: qsTr("Save chart changes")
            visible: !itemFact.newItem
            icon: "check-circle"
            onTriggered: itemFact.saveAll()
        }
    }

    Fact {
        id: mWarning
        name: "warning"
        title: qsTr("Warning")
        descr: qsTr("Expression that raises a page warning")
        flags: Fact.Text
    }

    Fact {
        id: mSaveFact
        name: "save"
        title: qsTr("Save target")
        descr: qsTr("Pick a mandala fact under sns.scr")
        flags: Fact.Int
        units: "mandala"
        onTextChanged: {
            itemFact.refreshValidation()
            if (itemFact.setFact && typeof itemFact.setFact.refreshSaveWarnings === "function")
                itemFact.setFact.refreshSaveWarnings()
        }
    }

    Fact {
        title: qsTr("Save target status")
        descr: qsTr("Fix this before saving")
        visible: itemFact.saveError !== ""
        icon: "alert-circle"
        value: itemFact.saveError
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        descr: qsTr("Save chart changes")
        visible: !itemFact.newItem
        icon: "check-circle"
        onTriggered: itemFact.saveAll()
    }

    Fact {
        flags: (Fact.Action | Fact.Apply | Fact.ShowDisabled)
        title: qsTr("Add")
        descr: qsTr("Add this item")
        enabled: itemFact.newItem && itemFact.canSave()
        icon: "plus-circle"
        onTriggered: {
            itemFact.menuBack()
            itemFact.addTriggered()
        }
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        descr: qsTr("Remove this item")
        visible: !itemFact.newItem
        icon: "delete"
        onTriggered: {
            var ownerSet = itemFact.setFact
            itemFact.removeTriggered()
            itemFact.deleteFact()
            if (ownerSet && typeof ownerSet.refreshSaveWarnings === "function")
                ownerSet.refreshSaveWarnings()
        }
    }
}
