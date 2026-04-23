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
    id: menuPage

    flags: (Fact.Group | Fact.FlatModel)

    // Page properties
    property bool pinned: false
    property real speed: 1.0

    // Values list for the chart renderer — array of MenuItem objects
    property var values: []

    // Warning/alarm aggregated from items
    property bool hasWarning: false
    property bool hasAlarm: false
    property string warningText: ""

    // Set to true when created directly from Signals.qml (page-button flow)
    // to show the per-page Save button. False when used inside SignalsMenu popup.
    property bool isDirectEdit: false

    Component.onCompleted: {
        pTitle.value = title;
    }

    function addNewItem() {
        mMenuNewItem.trigger();
    }

    // Rebuild the values array from current items
    function updatePageValues() {
        var list = [];
        for (var i = 0; i < mItems.size; ++i) {
            var it = mItems.child(i);
            list.push(it);
        }
        values = list;
        // Rebuild opts colors for chart
        updateWarnings();
    }

    function updateWarnings() {
        var warn = false;
        var alarm = false;
        var message = "";
        for (var i = 0; i < mItems.size; ++i) {
            var it = mItems.child(i);
            if (it.hasWarning) {
                warn = true;
                if (message === "")
                    message = it.alertText || it.warningMsg;
            }
            if (it.hasAlarm) {
                alarm = true;
                if (message === "")
                    message = it.alertText || it.warningMsg;
            }
        }
        hasWarning = warn;
        hasAlarm = alarm;
        warningText = message;
    }

    function updateChartsValues() {
        for (var i = 0; i < mItems.size; ++i)
            mItems.child(i).updateValue();
        updateWarnings();
    }

    function setSpeed(value) {
        if (mSpeed.value === value)
            return;
        mSpeed.value = value;
    }

    function save() {
        var items = [];
        for (var i = 0; i < mItems.size; ++i) {
            var item = mItems.child(i).save();
            if (!item.bind)
                continue;
            items.push(item);
        }
        return {
            name: pTitle.value,
            pin: mPin.value ? true : false,
            speed: mSpeed.value,
            items: items
        };
    }

    function load(pageData) {
        pTitle.value = pageData.name ? pageData.name : title;
        pinned = pageData.pin ? true : false;
        mPin.value = pinned;
        speed = pageData.speed !== undefined ? pageData.speed : 1.0;
        mSpeed.value = speed;
        mItems.deleteChildren();
        var items = pageData.items ? pageData.items : [];
        for (var i in items)
            createItem(items[i]);
        updatePageValues();
    }

    function createItem(itemData) {
        if (!itemData.bind || itemData.bind === "")
            return;
        var wasEmpty = mItems.size === 0;
        var c = createFact(mItems, "MenuItem.qml", { "data": itemData });
        c.parentFact = mItems;
        c.removeTriggered.connect(function () { updatePageValues(); });
        c.titleChanged.connect(updatePageValues);
        if (wasEmpty && (!pTitle.value || /^P\d+$/.test(pTitle.value)))
            pTitle.value = defaultPageName(itemData.bind);
        return c;
    }

    function defaultPageName(bindExpr) {
        if (!bindExpr || bindExpr === "")
            return "P";
        var expr = bindExpr.replace(/^mandala\./, "").replace(/\.value$/, "");
        var parts = expr.split(".");
        var leaf = parts.length > 0 ? parts[parts.length - 1] : expr;
        return leaf.length > 0 ? leaf.charAt(0).toUpperCase() : "P";
    }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            return c;
        }
        console.warn("MenuPage.createFact: failed to load " + url + ": " + component.errorString());
    }

    function checkScrs(val) {
        var matches = false;
        for (var i = 0; i < mItems.size; ++i)
            if (mItems.child(i).hasScr(val))
                matches = true;
        return matches;
    }

    // Page title fact
    Fact {
        id: pTitle
        title: qsTr("Page name")
        descr: qsTr("Short name shown on tab")
        flags: Fact.Text
        icon: "rename-box"
        onValueChanged: {
            menuPage.title = value;
        }
    }
    Fact {
        id: mPin
        name: "pin"
        title: qsTr("Pinned")
        descr: qsTr("Show this page stacked with other pinned pages")
        flags: Fact.Bool
        onValueChanged: {
            menuPage.pinned = value > 0;
            if (typeof signalsWidget !== 'undefined' && signalsWidget)
                signalsWidget.updateLayout();
        }
    }
    Fact {
        id: mSpeed
        name: "speed"
        title: qsTr("Speed")
        descr: qsTr("Chart scroll speed factor")
        flags: Fact.Float
        enumStrings: ["0.2", "0.5", "1", "2", "4"]
        value: 1.0
        precision: 1
        onValueChanged: {
            menuPage.speed = value;
        }
    }

    // Add new item action
    MenuItem {
        id: mMenuNewItem
        title: qsTr("Add new chart")
        descr: qsTr("Create and configure a new chart item")
        icon: "plus-circle"
        newItem: true
        onAddTriggered: createItem(save())
    }

    Fact {
        id: mItems
        title: qsTr("Items")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: {
            menuPage.updatePageValues();
            if (typeof signalsWidget !== 'undefined' && signalsWidget)
                signalsWidget.updateLayout();
        }
        onItemMoved: menuPage.updatePageValues()
    }

    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        visible: menuPage.isDirectEdit
        icon: "check-circle"
        onTriggered: {
            if (typeof signalsWidget !== 'undefined' && signalsWidget)
                signalsWidget.saveSettings();
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove page")
        icon: "delete"
        onTriggered: menuPage.deleteFact()
    }
}
