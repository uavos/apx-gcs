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
    id: setFact

    property var signalsModel: null
    property var data: ({})

    flags: (Fact.Group | Fact.FlatModel)
    title: setTitleText()
    descr: pagesSummary()

    signal selected(var num)
    signal stateChanged()

    Component.onCompleted: {
        load()
        refreshSaveWarnings()
    }

    function rootEditor()
    {
        for (var parent = setFact.parentFact; parent; parent = parent.parentFact) {
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

    function defaultTitle()
    {
        if (signalsModel && typeof signalsModel.defaultSetTitle === "function")
            return setFact.signalsModel.defaultSetTitle(Math.max(setFact.num, 0))
        return qsTr("Set") + " " + (Math.max(setFact.num, 0) + 1)
    }

    function pageFacts()
    {
        var pages = []

        for (var i = 0; i < setPages.size; ++i)
            pages.push(setPages.child(i))

        return pages
    }

    function load()
    {
        setTitle.value = data && data.title !== undefined ? data.title : defaultTitle()
        updatePages()
    }

    function setTitleText()
    {
        return setTitle.text.trim() !== "" ? setTitle.text.trim() : setFact.defaultTitle()
    }

    function save()
    {
        refreshSaveWarnings()

        var pages = []
        for (var i = 0; i < setPages.size; ++i) {
            var pageEditor = setPages.child(i)
            var pageData = pageEditor.save()
            if (pageData === null)
                return null
            pages.push(pageData)
        }

        return {
            "title": setTitleText(),
            "pages": pages
        }
    }

    function updatePages()
    {
        setPages.deleteChildren()

        var pages = data && data.pages instanceof Array ? data.pages : []
        for (var i = 0; i < pages.length; ++i)
            createPage(pages[i])
    }

    function createPage(pageData)
    {
        var child = createFact(setPages, "MenuPage.qml", {
                                   "data": pageData,
                                   "signalsModel": setFact.signalsModel,
                                   "setFact": setFact
                               })
        if (!child)
            return null
        return child
    }

    function refreshSaveWarnings()
    {
        for (var i = 0; i < setPages.size; ++i) {
            var pageEditor = setPages.child(i)
            if (!pageEditor || !pageEditor.itemsFact)
                continue

            for (var j = 0; j < pageEditor.itemsFact.size; ++j) {
                var itemEditor = pageEditor.itemsFact.child(j)
                if (itemEditor && typeof itemEditor.refreshValidation === "function")
                    itemEditor.refreshValidation()
            }
        }
    }

    function isSaveTargetUsed(saveTarget, skipItem)
    {
        var target = String(saveTarget).trim()
        if (target === "")
            return false

        for (var i = 0; i < setPages.size; ++i) {
            var pageEditor = setPages.child(i)
            if (!pageEditor || !pageEditor.itemsFact)
                continue

            for (var j = 0; j < pageEditor.itemsFact.size; ++j) {
                var itemEditor = pageEditor.itemsFact.child(j)
                if (!itemEditor || itemEditor === skipItem)
                    continue
                if (typeof itemEditor.saveTarget === "function"
                        && itemEditor.saveTarget() === target)
                    return true
            }
        }

        return false
    }

    function pagesSummary()
    {
        var pages = []
        for (var i = 0; i < setPages.size; ++i)
            pages.push(setPages.child(i).title)
        return pages.join(", ")
    }

    Fact {
        id: setTitle
        title: qsTr("Set name")
        descr: qsTr("Saved chart configuration name")
        flags: Fact.Text
        icon: "rename-box"
    }

    MenuPage {
        title: qsTr("Add new page")
        icon: "plus-circle"
        newItem: true
        signalsModel: setFact.signalsModel
        setFact: setFact
        onAddTriggered: {
            var pageData = save()
            if (pageData) {
                setFact.createPage(pageData)
                setFact.stateChanged()
            }
        }
    }

    Fact {
        id: setPages
        title: qsTr("Pages")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove set")
        descr: qsTr("Delete this chart set")
        icon: "delete"
        onTriggered: {
            if (setFact.active)
                selected(0)
            setFact.stateChanged()
            setFact.deleteFact()
        }
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        descr: qsTr("Save chart changes")
        icon: "check-circle"
        onTriggered: setFact.saveAll()
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Select")
        descr: qsTr("Make this set active")
        visible: !setFact.active
        icon: "check-circle"
        onTriggered: {
            setFact.menuBack()
            setFact.selected(setFact.num)
        }
    }
}
