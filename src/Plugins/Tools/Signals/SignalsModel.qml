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
    id: model

    /*
     * signals.json schema:
     * {
     *   "active": { "signals": 0 },
     *   "sets": [
     *     {
     *       "title": "default",
     *       "pages": [
     *         {
     *           "name": "R",
     *           "pin": false,
     *           "speed": 1.0,
     *           "items": [
     *             {
     *               "bind": "est.att.roll",
     *               "color": "",
     *               "filters": [],
     *               "warning": "",
     *               "save": ""
     *             }
     *           ]
     *         }
     *       ]
     *     }
     *   ]
     * }
     *
     * Legacy migration:
     * { "page": "R", "signalas": [ ... ] } becomes one set titled "default"
     * with one page named from "page" (or "page 1" when missing). Legacy item
     * fields are copied forward and normalized into the new item schema.
     */

    property string settingsFileName: "signals.json"
    property string settingsName: "signals"
    property bool autoLoad: true

    // Provided by the host widget or a test harness.
    property var prefsAdapter: null

    property var speedFactors: [0.2, 0.5, 1.0, 2.0, 4.0]
    property var active: ({signals: 0})
    property int activeSignals: -1
    property var sets: []
    property bool loaded: false
    property var filterRegistry: FilterRegistry {}

    readonly property bool hasSets: sets.length > 0
    readonly property var activeSet: hasSets ? sets[activeSignals] : null
    readonly property var pages: activeSet && (activeSet.pages instanceof Array) ? activeSet.pages : []
    readonly property var pinnedPages: filterPages(true)
    readonly property var unpinnedPages: filterPages(false)

    signal settingsLoaded(var settings)
    signal settingsSaved(var settings)

    Component.onCompleted: {
        if (autoLoad)
            loadSettings()
    }

    function settingsStore()
    {
        return prefsAdapter
    }

    function defaultSetTitle(index)
    {
        return qsTr("Set") + " " + (index + 1)
    }

    function defaultPageTitle(index)
    {
        return qsTr("Page") + " " + (index + 1)
    }

    function legacyDefaultPageTitle()
    {
        return qsTr("page 1")
    }

    function createDefaultItem(bind)
    {
        return {
            "bind": bind,
            "color": "",
            "filters": [],
            "warning": "",
            "save": ""
        }
    }

    function createDefaultPage(name, bindings)
    {
        var page = {
            "name": name,
            "pin": false,
            "speed": 1.0,
            "items": []
        }

        for (var i = 0; i < bindings.length; ++i)
            page.items.push(createDefaultItem(bindings[i]))

        return page
    }

    function createDefaultSet()
    {
        var specs = [
            {
                "name": "R",
                "bindings": ["cmd.att.roll", "est.att.roll"]
            },
            {
                "name": "P",
                "bindings": ["cmd.att.pitch", "est.att.pitch"]
            },
            {
                "name": "Y",
                "bindings": ["cmd.pos.bearing", "cmd.att.yaw", "est.att.yaw"]
            },
            {
                "name": "Axy",
                "bindings": ["est.acc.x", "est.acc.y"]
            },
            {
                "name": "Az",
                "bindings": ["est.acc.z"]
            },
            {
                "name": "G",
                "bindings": ["est.gyro.x", "est.gyro.y", "est.gyro.z"]
            },
            {
                "name": "Pt",
                "bindings": ["est.pos.altitude", "est.pos.vspeed", "est.air.airspeed"]
            },
            {
                "name": "Ctr",
                "bindings": [
                    "ctr.att.ail",
                    "ctr.att.elv",
                    "ctr.att.rud",
                    "ctr.eng.thr",
                    "ctr.eng.prop",
                    "ctr.str.rud"
                ]
            },
            {
                "name": "RC",
                "bindings": ["cmd.rc.roll", "cmd.rc.pitch", "cmd.rc.thr", "cmd.rc.yaw"]
            },
            {
                "name": "Usr",
                "bindings": [
                    "est.usr.u1",
                    "est.usr.u2",
                    "est.usr.u3",
                    "est.usr.u4",
                    "est.usr.u5",
                    "est.usr.u6"
                ]
            }
        ]
        var set = {
            "title": "default",
            "pages": []
        }

        for (var i = 0; i < specs.length; ++i)
            set.pages.push(createDefaultPage(specs[i].name, specs[i].bindings))

        return set
    }

    function createDefaultSettings()
    {
        return {
            "active": {
                "signals": 0
            },
            "sets": [createDefaultSet()]
        }
    }

    function copyValue(value)
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

    function asArray(value)
    {
        return value instanceof Array ? value : []
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

    function normalizeSpeed(value)
    {
        var number = asNumber(value, 1.0)

        for (var i = 0; i < speedFactors.length; ++i) {
            if (speedFactors[i] === number)
                return number
        }

        return 1.0
    }

    function speedIndex(value)
    {
        var speedValue = normalizeSpeed(value)

        for (var i = 0; i < speedFactors.length; ++i) {
            if (speedFactors[i] === speedValue)
                return i
        }

        return 2
    }

    function nextSpeedValue(value)
    {
        var index = speedIndex(value)
        return speedFactors[(index + 1) % speedFactors.length]
    }

    function formatSpeed(value)
    {
        var text = String(normalizeSpeed(value))
        if (text.endsWith(".0"))
            text = text.slice(0, -2)
        return text + "x"
    }

    function normalizeActiveIndex(index, count)
    {
        var number = Math.floor(asNumber(index, 0))

        if (count <= 0)
            return 0

        if (number < 0 || number >= count)
            return 0

        return number
    }

    function parseSettingsText(text)
    {
        if (!text)
            return {}

        try {
            return JSON.parse(text)
        } catch (error) {
            console.warn("SignalsModel: invalid " + settingsFileName + ": " + error)
        }

        return {}
    }

    function isLegacySettings(json)
    {
        if (!json || typeof json !== "object")
            return false

        if (json.sets instanceof Array)
            return false

        return json.signalas instanceof Array || json.page !== undefined
    }

    function normalizeFilter(filterData)
    {
        return filterRegistry.normalizeFilter(filterData)
    }

    function normalizeFilters(filtersData)
    {
        return filterRegistry.normalizeFilters(filtersData)
    }

    function normalizeBind(value)
    {
        var text = asString(value, "").trim()
        var simplePath = /^(?:mandala\.)?[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+(?:\.value)?$/

        if (text === "" || !simplePath.test(text))
            return text

        if (text.startsWith("mandala."))
            text = text.slice(8)
        if (text.endsWith(".value"))
            text = text.slice(0, -6)

        return text
    }

    function normalizeItem(itemData)
    {
        var source = asObject(itemData)
        var item = copyValue(source) || {}

        item.bind = normalizeBind(source.bind !== undefined ? source.bind : source.name)
        if (!item.bind)
            return null

        item.color = asString(source.color, "")
        item.filters = normalizeFilters(source.filters)
        item.warning = asString(source.warning !== undefined ? source.warning : source.warn, "")
        item.save = asString(source.save, "")

        delete item.title
        delete item.act
        delete item.warn
        delete item.name
        delete item.descr
        delete item.value

        if (item.opts && item.color === "")
            item.color = asString(item.opts.color, "")

        delete item.opts

        return item
    }

    function normalizeItems(itemsData)
    {
        var list = []
        var source = asArray(itemsData)

        for (var i = 0; i < source.length; ++i) {
            var item = normalizeItem(source[i])
            if (item)
                list.push(item)
        }

        return list
    }

    function normalizePage(pageData, index)
    {
        var source = asObject(pageData)
        var page = copyValue(source) || {}

        page.name = asString(source.name, defaultPageTitle(index))
        page.pin = asBool(source.pin, false)
        page.speed = normalizeSpeed(source.speed)
        page.items = normalizeItems(source.items)

        return page
    }

    function normalizePages(pagesData)
    {
        var list = []
        var source = asArray(pagesData)

        for (var i = 0; i < source.length; ++i)
            list.push(normalizePage(source[i], i))

        return list
    }

    function normalizeSet(setData, index)
    {
        var source = asObject(setData)
        var set = copyValue(source) || {}

        set.title = asString(source.title, defaultSetTitle(index))
        set.pages = normalizePages(source.pages)

        return set
    }

    function normalizeSets(setsData)
    {
        var list = []
        var source = asArray(setsData)

        for (var i = 0; i < source.length; ++i)
            list.push(normalizeSet(source[i], i))

        return list
    }

    function normalizeLegacySettings(json)
    {
        var source = asObject(json)

        return {
            "active": {
                "signals": 0
            },
            "sets": [
                {
                    "title": "default",
                    "pages": [
                        {
                            "name": asString(source.page, legacyDefaultPageTitle()),
                            "pin": false,
                            "speed": 1.0,
                            "items": normalizeItems(source.signalas)
                        }
                    ]
                }
            ]
        }
    }

    function normalizeSettings(json)
    {
        var root = isLegacySettings(json) ? normalizeLegacySettings(json) : asObject(json)
        var normalizedSets = normalizeSets(root.sets)

        if (normalizedSets.length <= 0)
            normalizedSets = normalizeSets([createDefaultSet()])

        var normalized = {
            "active": {
                "signals": 0
            },
            "sets": normalizedSets
        }

        normalized.active.signals = normalizeActiveIndex(asObject(root.active).signals,
                                                         normalized.sets.length)
        return normalized
    }

    function assignSettings(normalized)
    {
        sets = normalized.sets
        active = normalized.active
        activeSignals = normalized.active.signals
    }

    function applySettings(settings)
    {
        var normalized = normalizeSettings(settings)

        assignSettings(normalized)
        loaded = true

        return exportSettings()
    }

    function exportSettings()
    {
        return normalizeSettings({
                                     "active": {
                                         "signals": hasSets ? activeSignals : 0
                                     },
                                     "sets": sets
                                 })
    }

    function loadSettings()
    {
        var store = settingsStore()
        var text = ""
        var settings = createDefaultSettings()

        if (store && typeof store.loadFile === "function") {
            text = store.loadFile(settingsFileName)
            if (text)
                settings = parseSettingsText(text)
        }

        var exported = applySettings(settings)
        settingsLoaded(exported)
        return exported
    }

    function saveSettings(settings)
    {
        if (settings !== undefined)
            applySettings(settings)

        var exported = exportSettings()
        var store = settingsStore()

        if (store && typeof store.saveFile === "function")
            store.saveFile(settingsFileName, JSON.stringify(exported, " ", 2))

        settingsSaved(exported)
        return exported
    }

    function setActiveSignals(index)
    {
        assignSettings(normalizeSettings({
                                             "active": {
                                                 "signals": index
                                             },
                                             "sets": sets
                                         }))
    }

    function setSets(list)
    {
        assignSettings(normalizeSettings({
                                             "active": {
                                                 "signals": activeSignals
                                             },
                                             "sets": list
                                         }))
    }

    function mutateActiveSet(mutator)
    {
        if (!hasSets || typeof mutator !== "function")
            return exportSettings()

        var settings = exportSettings()
        var setIndex = normalizeActiveIndex(settings.active.signals, settings.sets.length)
        var setData = setIndex >= 0 && setIndex < settings.sets.length ? settings.sets[setIndex] : null
        if (!setData)
            return settings

        mutator(setData)
        return saveSettings(settings)
    }

    function pageAt(index)
    {
        return index >= 0 && index < pages.length ? pages[index] : null
    }

    function setPageSpeed(index, speedValue)
    {
        return mutateActiveSet(function(setData) {
            if (!setData.pages || index < 0 || index >= setData.pages.length)
                return

            setData.pages[index].speed = normalizeSpeed(speedValue)
        })
    }

    function togglePagePin(index)
    {
        return mutateActiveSet(function(setData) {
            if (!setData.pages || index < 0 || index >= setData.pages.length)
                return

            setData.pages[index].pin = !setData.pages[index].pin
        })
    }

    function updatePage(index, pageData)
    {
        return mutateActiveSet(function(setData) {
            if (!setData.pages || index < 0 || index >= setData.pages.length)
                return

            setData.pages[index] = normalizePage(pageData, index)
        })
    }

    function removePage(index)
    {
        return mutateActiveSet(function(setData) {
            if (!setData.pages || index < 0 || index >= setData.pages.length)
                return

            setData.pages.splice(index, 1)
            if (setData.pages.length <= 0) {
                setData.pages.push(normalizePage({
                                                     "name": defaultPageTitle(0),
                                                     "pin": false,
                                                     "speed": 1.0,
                                                     "items": []
                                                 }, 0))
            }
        })
    }

    function addItemToPage(pageIndex, itemData)
    {
        return mutateActiveSet(function(setData) {
            if (!setData.pages || pageIndex < 0 || pageIndex >= setData.pages.length)
                return

            var item = normalizeItem(itemData)
            if (!item)
                return

            setData.pages[pageIndex].items.push(item)
        })
    }

    function isSaveTargetUsedInActiveSet(saveTarget, skipPageIndex, skipItemIndex)
    {
        var target = asString(saveTarget, "").trim()
        if (target === "")
            return false

        if (skipPageIndex === undefined)
            skipPageIndex = -1
        if (skipItemIndex === undefined)
            skipItemIndex = -1

        for (var i = 0; i < pages.length; ++i) {
            if (i === skipPageIndex && skipItemIndex < 0)
                continue

            var page = pages[i]
            var items = page && page.items instanceof Array ? page.items : []
            for (var j = 0; j < items.length; ++j) {
                if (i === skipPageIndex && j === skipItemIndex)
                    continue

                var item = items[j]
                if (asString(item ? item.save : "", "").trim() === target)
                    return true
            }
        }

        return false
    }

    function filterPages(pinned)
    {
        var list = []

        for (var i = 0; i < pages.length; ++i) {
            var page = pages[i]
            if (!!page.pin === pinned)
                list.push(page)
        }

        return list
    }
}
