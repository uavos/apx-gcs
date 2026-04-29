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

    property bool newItem: false
    // property bool changes: false
    property var pages //from config
    property var data: ({})

    signal selected(var num)
    signal removeTriggered

    Component.onCompleted: load(data)
    Component.onDestruction: removed() // pinned menu closes when the plugin is closed

    // function addNewPage() {
    //     mMenuPage.trigger();
    // }

    // function updateBtnValues() {
    //     sgBtn.values = [];
    //     for (var i = 0; i < mPages.size; ++i)
    //         sgBtn.values.push(mPages.child(i));
    //     sgBtn.updateToolTip(sgBtn.values);
    // }

    // function updateChartsValues() {
    //     for (var i = 0; i < mPages.size; ++i)
    //         mPages.child(i).updateValue();
    // }

    function save() {
        // changes = false;
        var tmpPages = [];
        for (var i = 0; i < mPages.size; ++i) {
            var mpage = mPages.child(i).save();
            tmpPages.push(mpage);
        }
        var set = {};
        set.title = mSetName.value;
        set.pages = tmpPages;
        return set;
    }

    function load(set) {
        mSetName.value = set.title;
        pages = set.pages;
        updateSetItems();
        // changes = false;
    }

    function updateSetItems() {
        mPages.deleteChildren();
        for (var i in pages) {
            createPage(pages[i]);
        }
    }

    function createPage(mset) {
        // function createPage() {
        var text = mset.title.trim();
        if (text == "")
            mset.title = "P" + (mPages.size + 1);
        if (mset.speed <= 0)
            mset.speed = 1;
        var c = createFact(mPages, "SignalsMenuPage.qml", {
            "data": mset
        });
        // var page = {};
        // page.values = [];
        // var c = createFact(mPages, "SignalsMenuPage.qml", page);
        // c.trigger();

        // c.removeTriggered.connect(function () {
        // // changes = true;
        // });
        // // changes = true;
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

    function getActivePage() {
        for (var i = 0; i < mPages.size; ++i) {
            var f = mPages.child(i);
            if (!f)
                continue;
            if(f.active)
                return f;
        }
        return null;
    }

    Fact {
        id: mSetName
        title: qsTr("Set name")
        descr: qsTr("Saved chart configuration name")
        flags: Fact.Text
        icon: "rename-box"
        onValueChanged: {
            setFact.title = value;
            // changes = true;
        }
    }
    SignalsMenuPage {
        id: mMenuPage
        title: qsTr("Add new page")
        descr: qsTr("Creating and add new page")
        icon: "plus-circle"
        newItem: true
        // onAddTriggered: createPage()
        onAddTriggered: createPage(save())
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
        visible: !newItem // && changes
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
        visible: !newItem
        icon: "delete"
        onTriggered: {
            if (setFact.num <= 0) {  // don't delete default set
                console.warn(qsTr("Can't delete default set"));
                return;
            }
            removeTriggered();
            setFact.deleteFact();
        }
    }
}
