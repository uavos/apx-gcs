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
    // property var values //from config
    property var values

    signal removeTriggered

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
        var tmpValues = [];
        for (var i = 0; i < mPages.size; ++i) {
            var mpage = mPages.child(i).save();
            tmpPages.push(mpage);
        }
        var set = {};
        set.title = mSetName.value;
        set.values = tmpValues;
        return set;
    }

    function load(set) {
        // mSetName.value = set.title;
        // mSpeed.value = set.speed;
        // values = set.values;
        // updateSetItems();
        // changes = false;
    }

    function updateSetItems() {
        // mPages.deleteChildren();
        // for (var i in values) {
        //     createPage(values[i]);
        // }
    }

    function createPage(mset) {
    // function createPage() {
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

    Fact {
        id: mSetName
        title: qsTr("Set name")
        descr: qsTr("Saved chart configuration name")
        flags: Fact.Text
        icon: "rename-box"
        value: setFact.title
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
        // onSizeChanged: sgCharts.resetEnable = true
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        visible: !newItem // && changes
        icon: "check-circle"
        onTriggered: console.log("Not implemented")
        // onTriggered: sgControl.saveSettings()
    }
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Select and save")
        visible: !setFact.active
        icon: "check-circle"
        onTriggered: {
            setFact.menuBack();
            // setFact.selected(setFact.num);
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        visible: !newItem
        icon: "delete"
        onTriggered: {
            removeTriggered();
            setFact.deleteFact();
        }
    }
}
