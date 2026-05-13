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
    id: setFact

    flags: (Fact.Group | Fact.FlatModel)
    title: "Set"

    property bool changes: false
    property var pages: [] //from config
    property var checkedPage: 0
    property var data: ({})

    readonly property var pagesLimit: 10

    signal selected(var num)

    Component.onCompleted: load(data)
    Component.onDestruction: removed() // pinned menu closes when the plugin is closed
    onCheckedPageChanged: if(active) sgMenu.saveSettings() 

    function save() {
        changes = false;
        var tmpPages = [];
        for (var i = 0; i < mPages.size; ++i) {
            var mpage = mPages.child(i).save();
            tmpPages.push(mpage);
        }
        var set = {};
        set.title = mSetName.value;
        set.checked = checkedPage;
        set.pages = tmpPages;
        return set;
    }

    function load(set) {
        mSetName.value = set.title;
        checkedPage = set.checked;
        pages = set.pages;
        updateSetItems();
        setChecked(checkedPage);
        changes = false;
    }

    function updateSetItems() {
        mPages.deleteChildren();
        for (var i in pages) {
            createPage(pages[i]);
        }
    }

    function setChecked(num) {
        for (var i = 0; i < mPages.size; ++i) {
            mPages.child(i).active = i == num;
        } 
    }

    function createPage(mpage) {
        var text = mpage.title.trim();
        if (text == "")
            mpage.title = "P" + (mPages.size + 1);
        if (mpage.speed <= 0)
            mpage.speed = 1;
        var c = createFact(mPages, "SignalsMenuPage.qml", {
            "data": mpage
        });
    }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            return c;
        }
    }

    function updateDescr() {
        var descrList = [];
        for (var i = 0; i < mPages.size; ++i) {
            var f = mPages.child(i);
            if (!f)
                continue;
            var text = f.title ? f.title.trim() : "";
            descrList.push(text);
        }
        descr = descrList.length > 0 ? descrList.join(", ") : "";
        changes = true;
    }

    function getPinned() {
        var pinnedPages = [];
        for (var i = 0; i < mPages.size; ++i) {
            var f = mPages.child(i);
            if (!f)
                continue;
            if (!f.pinned)
                continue;
            pinnedPages.push(f);
        }
        return pinnedPages.length > 0 ? pinnedPages : [];
    }

    function getPages() {
        var pages = [];
        for (var i = 0; i < mPages.size; ++i) {
            var f = mPages.child(i);
            if (!f)
                continue;
            pages.push(f);
        }
        return pages.length > 0 ? pages : [];
    }

    // Check for binding script variable matches
    function checkScrMatches(val) {
        for (var i = 0; i < mPages.size; ++i)
            mPages.child(i).checkScrs(val)
    }

    Fact {
        id: mSetName
        title: qsTr("Set name")
        descr: qsTr("Saved chart configuration name")
        flags: Fact.Text
        icon: "rename-box"
        onValueChanged: {
            setFact.title = value;
            changes = true;
        }
    }
    SignalsMenuPage {
        id: mMenuPage
        title: qsTr("Add new page")
        descr: qsTr("Creating and add new page")
        icon: "plus-circle"
        newItem: true
        onAddTriggered: {
            if (mPages.size >= pagesLimit) {
                console.warn(qsTr("Maximum page limit reached"))
                return;
            }
            createPage(save());
            if(mPages.size === 1) setChecked(0);
        }
    }
    Fact {
        id: mPages
        title: qsTr("Pages")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: updateDescr()
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        visible: changes
        icon: "check-circle"
        onTriggered: sgMenu.saveSettings()
    }
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Select")
        visible: !setFact.active
        icon: "text-box-check"
        onTriggered: {
            setFact.selected(setFact.num);
            setFact.menuBack();
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        icon: "delete"
        onTriggered: {
            if (setFact.active) setFact.selected(0);
            setFact.deleteFact();
        }
    }
}
