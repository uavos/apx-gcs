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
    id: setsFact

    property var signalsModel: null
    property bool destroyOnClose: true
    property string defaultDescr: qsTr("Chart configuration editor")
    property bool loading: false

    name: setsFact.signalsModel && setsFact.signalsModel.settingsName ? setsFact.signalsModel.settingsName : "signals"
    flags: (Fact.Group | Fact.FlatModel)
    title: qsTr("Signals")
    descr: setsFact.defaultDescr
    icon: "poll"

    signal accepted()
    signal stateChanged()

    Component.onCompleted: {
        loadSettings()
    }

    function close()
    {
        if (!setsFact.destroyOnClose) {
            setsFact.menuBack()
            return
        }

        setsList.deleteChildren()
        setsFact.menuBack()
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

    function createSet(setData)
    {
        var child = createFact(setsList, "MenuSet.qml", {
                                   "data": setData,
                                   "signalsModel": setsFact.signalsModel
                               })
        if (!child)
            return null

        child.selected.connect(select)
        child.stateChanged.connect(markChanged)
        return child
    }

    function activeSetIndex()
    {
        for (var i = 0; i < setsList.size; ++i) {
            if (setsList.child(i).active)
                return i
        }

        return setsList.size > 0 ? 0 : -1
    }

    function activeSetFact()
    {
        var index = activeSetIndex()
        return index >= 0 && index < setsList.size ? setsList.child(index) : null
    }

    function activePages()
    {
        var activeSet = activeSetFact()
        return activeSet && typeof activeSet.pageFacts === "function" ? activeSet.pageFacts() : []
    }

    function activeSetTitle()
    {
        var activeSet = activeSetFact()
        return activeSet ? activeSet.title : ""
    }

    function exportSettings()
    {
        var settings = {
            "active": {
                "signals": 0
            },
            "sets": []
        }

        for (var i = 0; i < setsList.size; ++i) {
            var setEditor = setsList.child(i)
            var setData = setEditor.save()
            if (setData === null)
                return null

            settings.sets.push(setData)
            if (setEditor.active)
                settings.active.signals = i
        }

        if (settings.sets.length <= 0) {
            var defaults = defaultSettings()
            settings.sets = defaults.sets instanceof Array ? defaults.sets : []
            settings.active.signals = defaults.active ? defaults.active.signals : 0
        }

        return settings
    }

    function markChanged()
    {
        if (loading)
            return

        stateChanged()
    }

    function loadTree(settings)
    {
        loading = true
        setsFact.descr = setsFact.defaultDescr
        setsList.deleteChildren()

        var sets = settings && settings.sets instanceof Array ? settings.sets : []
        var currentSetIdx = settings && settings.active ? settings.active.signals : 0

        for (var i = 0; i < sets.length; ++i)
            setsFact.createSet(sets[i])

        setsFact.select(currentSetIdx)
        loading = false
        markChanged()
    }

    function defaultSettings()
    {
        if (setsFact.signalsModel && typeof setsFact.signalsModel.createDefaultSettings === "function")
            return setsFact.signalsModel.createDefaultSettings()

        return {
            "active": {
                "signals": 0
            },
            "sets": []
        }
    }

    function loadSettings()
    {
        var settings = setsFact.defaultSettings()
        if (setsFact.signalsModel) {
            if (!setsFact.signalsModel.loaded && typeof setsFact.signalsModel.loadSettings === "function")
                setsFact.signalsModel.loadSettings()
            if (typeof setsFact.signalsModel.exportSettings === "function")
                settings = setsFact.signalsModel.exportSettings()
        }

        loadTree(settings)
    }

    function saveSettings()
    {
        setsFact.descr = setsFact.defaultDescr

        var settings = exportSettings()
        if (settings === null) {
            setsFact.descr = qsTr("Fix editor errors before saving")
            return
        }

        if (setsFact.signalsModel && typeof setsFact.signalsModel.saveSettings === "function")
            setsFact.signalsModel.saveSettings(settings)

        setsFact.accepted()
    }

    function resetToDefaults()
    {
        var settings = exportSettings()
        if (settings === null)
            return

        var defaults = defaultSettings()
        var defaultSet = defaults.sets instanceof Array && defaults.sets.length > 0
                         ? defaults.sets[0]
                         : null
        if (!defaultSet)
            return

        var targetIndex = -1
        for (var i = 0; i < settings.sets.length; ++i) {
            var title = String(settings.sets[i].title === undefined ? "" : settings.sets[i].title).trim().toLowerCase()
            if (title === "default") {
                targetIndex = i
                break
            }
        }

        if (targetIndex < 0) {
            settings.sets.unshift(defaultSet)
            targetIndex = 0
        } else {
            settings.sets[targetIndex] = defaultSet
        }

        settings.active.signals = targetIndex
        loadTree(settings)
    }

    function select(num)
    {
        for (var i = 0; i < setsList.size; ++i) {
            var setEditor = setsList.child(i)
            setEditor.active = setEditor.num === num
        }

        markChanged()
    }

    Fact {
        id: setsList
        title: qsTr("Sets")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
    }

    Fact {
        title: qsTr("Add set")
        descr: qsTr("Create a new chart set")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: {
            var setTitle = setsFact.signalsModel && typeof setsFact.signalsModel.defaultSetTitle === "function"
                           ? setsFact.signalsModel.defaultSetTitle(setsList.size)
                           : qsTr("Set") + " " + (setsList.size + 1)
            var child = setsFact.createSet({
                                      "title": setTitle,
                                      "pages": []
                                  })
            if (child) {
                child.trigger()
                markChanged()
            }
        }
    }

    Fact {
        title: qsTr("Reset to defaults")
        descr: qsTr("Replace the editor contents with the built-in default set")
        flags: Fact.Action
        icon: "restore"
        onTriggered: setsFact.resetToDefaults()
    }

    Fact {
        title: qsTr("Save")
        descr: qsTr("Save chart sets")
        flags: (Fact.Action | Fact.Apply)
        icon: "check-circle"
        onTriggered: setsFact.saveSettings()
    }
}
