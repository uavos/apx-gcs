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
import "."

Fact {
    id: pageFact

    property bool newItem: false
    property bool standaloneEditor: false
    property var data: ({})
    property var signalsModel: null
    property var setFact: null
    property real speedValue: 1.0
    property bool loading: false

    property alias itemsFact: pageItems

    flags: (Fact.Group | Fact.FlatModel)

    signal addTriggered()
    signal accepted(var pageData)
    signal removeTriggered()
    signal removedStandalone()

    Component.onCompleted: {
        load()
        updateTitle()
        updateDescr()
    }

    function rootEditor()
    {
        for (var parent = pageFact.parentFact; parent; parent = parent.parentFact) {
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

    function asNumber(value, fallback)
    {
        var number = Number(value)
        return isFinite(number) ? number : fallback
    }

    function defaultTitle()
    {
        if (signalsModel && typeof signalsModel.defaultPageTitle === "function")
            return pageFact.signalsModel.defaultPageTitle(Math.max(pageFact.num, 0))
        return qsTr("Page") + " " + (Math.max(pageFact.num, 0) + 1)
    }

    function pageNameFromBind(bind)
    {
        var text = String(bind).trim()
        var simplePath = /^(?:mandala\.)?[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+(?:\.value)?$/

        if (simplePath.test(text)) {
            if (text.startsWith("mandala."))
                text = text.slice(8)
            if (text.endsWith(".value"))
                text = text.slice(0, -6)
        }

        if (text === "")
            return ""

        var parts = text.split(".")
        var token = parts.length > 0 ? parts[parts.length - 1] : text
        if (token.length <= 0)
            return ""

        return token.charAt(0).toUpperCase()
    }

    function pageName()
    {
        var text = mName.text.trim()
        if (text !== "")
            return text

        if (pageItems.size > 0) {
            var firstItem = pageItems.child(0)
            if (firstItem && typeof firstItem.bindText === "function") {
                var guessed = pageNameFromBind(firstItem.bindText())
                if (guessed !== "")
                    return guessed
            }
        }

        return pageFact.defaultTitle()
    }

    function speedText()
    {
        if (signalsModel && typeof signalsModel.formatSpeed === "function")
            return signalsModel.formatSpeed(speedValue)

        var text = String(speedValue)
        if (text.endsWith(".0"))
            text = text.slice(0, -2)
        return text + "x"
    }

    function speedOptions()
    {
        var factors = signalsModel && signalsModel.speedFactors instanceof Array
                      ? signalsModel.speedFactors
                      : [0.2, 0.5, 1.0, 2.0, 4.0]
        var values = []

        for (var i = 0; i < factors.length; ++i) {
            if (signalsModel && typeof signalsModel.formatSpeed === "function")
                values.push(signalsModel.formatSpeed(factors[i]))
            else {
                var text = String(factors[i])
                if (text.endsWith(".0"))
                    text = text.slice(0, -2)
                values.push(text + "x")
            }
        }

        return values
    }

    function speedValueFromText(value)
    {
        var text = String(value === undefined || value === null ? "" : value).trim()
        var factors = signalsModel && signalsModel.speedFactors instanceof Array
                      ? signalsModel.speedFactors
                      : [0.2, 0.5, 1.0, 2.0, 4.0]

        for (var i = 0; i < factors.length; ++i) {
            var optionText = signalsModel && typeof signalsModel.formatSpeed === "function"
                             ? signalsModel.formatSpeed(factors[i])
                             : String(factors[i]).replace(/\.0$/, "") + "x"
            if (optionText === text)
                return Number(factors[i])
        }

        if (signalsModel && typeof signalsModel.normalizeSpeed === "function")
            return signalsModel.normalizeSpeed(text)

        return 1.0
    }

    function syncSpeedFact()
    {
        if (!mSpeed)
            return

        loading = true
        mSpeed.value = speedText()
        loading = false
    }

    function load()
    {
        loading = true
        mName.value = data && data.name !== undefined ? data.name : ""
        mPin.value = data && data.pin !== undefined ? data.pin : false
        speedValue = signalsModel && typeof signalsModel.normalizeSpeed === "function"
                     ? signalsModel.normalizeSpeed(data ? data.speed : undefined)
                     : asNumber(data ? data.speed : undefined, 1.0)
        syncSpeedFact()
        loading = false
        updateItems()
    }

    function updateItems()
    {
        pageItems.deleteChildren()

        var items = data && data.items instanceof Array ? data.items : []
        for (var i = 0; i < items.length; ++i)
            createItem(items[i])
    }

    function maybeAdoptNameFromBind(bind)
    {
        if (mName.text.trim() !== "")
            return

        var guessed = pageNameFromBind(bind)
        if (guessed !== "")
            mName.setValue(guessed)
    }

    function createItem(itemData)
    {
        var child = createFact(pageItems, "MenuItem.qml", {
                                   "data": itemData,
                                   "signalsModel": signalsModel,
                                   "setFact": setFact,
                                   "pageFact": pageFact
                               })
        if (!child)
            return null

        child.titleChanged.connect(updateDescr)
        child.removeTriggered.connect(updateDescr)

        if (itemData && itemData.bind !== undefined)
            maybeAdoptNameFromBind(itemData.bind)

        if (setFact && typeof setFact.refreshSaveWarnings === "function")
            setFact.refreshSaveWarnings()

        return child
    }

    function isSaveTargetUsedLocal(saveTarget, skipItem)
    {
        var target = String(saveTarget).trim()
        if (target === "")
            return false

        for (var i = 0; i < pageItems.size; ++i) {
            var itemEditor = pageItems.child(i)
            if (!itemEditor || itemEditor === skipItem)
                continue
            if (typeof itemEditor.saveTarget === "function"
                    && itemEditor.saveTarget() === target)
                return true
        }

        return false
    }

    function save()
    {
        var items = []
        for (var i = 0; i < pageItems.size; ++i) {
            var itemEditor = pageItems.child(i)
            var itemData = itemEditor.save()
            if (itemData === null)
                return null
            items.push(itemData)
        }

        return {
            "name": pageName(),
            "pin": mPin.value,
            "speed": speedValue,
            "items": items
        }
    }

    function updateTitle()
    {
        if (newItem)
            return

        title = pageName()
    }

    function updateDescr()
    {
        var parts = []
        if (mPin.value)
            parts.push(qsTr("Pinned"))

        parts.push(qsTr("Speed") + ": " + speedText())

        var items = []
        for (var i = 0; i < pageItems.size; ++i)
            items.push(pageItems.child(i).title)
        if (items.length > 0)
            parts.push(items.join(", "))

        descr = parts.join(", ")
    }

    Fact {
        id: mName
        title: qsTr("Page name")
        descr: qsTr("Tab label for this page")
        flags: Fact.Text
        icon: "rename-box"
        onValueChanged: {
            pageFact.updateTitle()
            pageFact.updateDescr()
        }
    }

    Fact {
        id: mPin
        title: qsTr("Pinned")
        descr: qsTr("Show this page in the stacked pinned layout")
        flags: Fact.Bool
        icon: "pin"
        onValueChanged: pageFact.updateDescr()
    }

    Fact {
        id: mSpeed
        title: qsTr("Speed")
        descr: qsTr("Default chart speed for this page")
        flags: Fact.Enum
        icon: "play-speed"
        enumStrings: pageFact.speedOptions()
        value: pageFact.speedText()
        onValueChanged: {
            if (pageFact.loading)
                return

            var selectedText = value === undefined || value === null ? text : value
            var selectedSpeed = pageFact.speedValueFromText(selectedText)
            if (selectedSpeed === pageFact.speedValue)
                return

            pageFact.speedValue = selectedSpeed
            pageFact.updateDescr()
            pageFact.syncSpeedFact()
        }
    }

    MenuItem {
        title: qsTr("Add new item")
        icon: "plus-circle"
        newItem: true
        visible: !pageFact.newItem
        signalsModel: pageFact.signalsModel
        setFact: pageFact.setFact
        pageFact: pageFact
        onAddTriggered: {
            var itemData = save()
            if (itemData)
                pageFact.createItem(itemData)
        }
    }

    Fact {
        id: pageItems
        title: qsTr("Items")
        visible: !pageFact.newItem
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        descr: pageFact.standaloneEditor ? qsTr("Apply page changes")
                                         : qsTr("Save chart changes")
        visible: !pageFact.newItem
        icon: "check-circle"
        onTriggered: {
            if (!pageFact.standaloneEditor) {
                pageFact.saveAll()
                return
            }

            var pageData = pageFact.save()
            if (pageData === null)
                return

            pageFact.accepted(pageData)
            pageFact.menuBack()
        }
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Add")
        descr: qsTr("Add this page")
        enabled: newItem
        icon: "plus-circle"
        onTriggered: {
            pageFact.menuBack()
            addTriggered()
        }
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        descr: qsTr("Remove this page")
        visible: !newItem
        icon: "delete"
        onTriggered: {
            if (pageFact.standaloneEditor) {
                removeTriggered()
                removedStandalone()
                pageFact.menuBack()
                return
            }

            var ownerSet = setFact
            removeTriggered()
            pageFact.deleteFact()
            if (ownerSet && typeof ownerSet.refreshSaveWarnings === "function")
                ownerSet.refreshSaveWarnings()
        }
    }
}
