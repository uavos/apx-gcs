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
    id: pageFact

    flags: (Fact.Group | Fact.FlatModel)

    property var newItem: false
    property bool changes: false
    property bool warnings: false
    property string pageToolTip: ""
    property alias speed: mSpeed.value
    property alias pinned: mPinned.value
    property var charts //from config
    property var values: []
    property var data: ({})

    signal addTriggered

    Component.onDestruction: removed() // pinned menu closes when the plugin is closed
    Component.onCompleted: {
        load(data);
        updateTitle();
        updateDescr();
    }
    onActiveChanged: {
        if (active) 
            setFact.checkedPage = num;
    }
    onPinnedChanged: setFact.updatePinnedPages()

    function addNewChart() {
        mMenuChart.trigger();
    }

    function updateBtnValues() {
        values = [];
        for (var i = 0; i < mCharts.size; ++i)
            values.push(mCharts.child(i));
        updateToolTip();
    }

    function updateChartsValues() {
        for (var i = 0; i < mCharts.size; ++i) {
            mCharts.child(i).updateValue();
            mCharts.child(i).updateWarning();
        }
        updatePageWarning();
    }

    function updatePageWarning() {
        for (var i = 0; i < mCharts.size; ++i) {
            if (mCharts.child(i).warning) {
                warnings = true;
                return;
            }
        }
        warnings = false;
    }

    function save() {
        changes = false;
        var tmpCharts = [];
        for (var i = 0; i < mCharts.size; ++i) {
            var mchart = mCharts.child(i).save();
            if (!mchart.bind)
                continue;
            tmpCharts.push(mchart);
        }
        var page = {};
        page.title = mPageName.value;
        page.pinned = mPinned.value;
        page.speed = mSpeed.value;
        page.charts = tmpCharts;
        updateBtnValues();
        return page;
    }

    function load(page) {
        mPageName.value = page.title;
        mSpeed.value = page.speed ? page.speed : 1;
        mPinned.value = page.pinned;
        charts = page.charts;
        updateSetItems();
        changes = false;
    }

    function updateSetItems() {
        mCharts.deleteChildren();
        for (var i in charts) {
            createChart(charts[i]);
        }
        updateBtnValues();
    }

    function createChart(mchart) {
        if (!mchart.bind)
            return;
        if (mchart.bind === "")
            return;
        var c = createFact(mCharts, "SignalsMenuChart.qml", {
            "data": mchart
        });
        changes = true;
    }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            return c;
        }
    }

    // Check for binding script variable matches
    function checkScrs(val) {
        for (var i = 0; i < mCharts.size; ++i)
            mCharts.child(i).hasScr(val)
    }

    function updateTitle() {
        if (newItem)
            return;
        var text = mPageName.text.trim();
        title = text != "" ? text : qsTr("P") + (Math.max(pageFact.num, 0) + 1);
        changes = true;
    }

    function updateDescr() {
        var descrList = [];
        if(mPinned.value)
             descrList.push("PINNED")
        var speedDescr = "SPEED: " + mSpeed.value;
        descrList.push(speedDescr);
        var descrCharts = [];
        for (var i = 0; i < mCharts.size; ++i) {
            var f = mCharts.child(i);
            if (!f)
                continue;
            var text = f.title ? f.title.trim() : "";
            descrCharts.push(text);
        }
        if (descrCharts.length > 0) {
            var chartsDescr = descrCharts.join(", ");
            chartsDescr = "CHARTS: " + chartsDescr;
            descrList.push(chartsDescr);
        }
        descr = descrList.length > 0 ? descrList.join(", ") : "";
        changes = true;
    }

    function updateToolTip() {
        var s = [];
        s.push("<strong>" + title + "</strong>");
        for (var i = 0; i < mCharts.size; ++i) {
            var fact = mCharts.child(i);
            var color = fact.color ? fact.color : (fact.opts ? fact.opts.color : undefined);
            var label = fact.title ? fact.title : (fact.descr ? fact.descr : fact.bind);
            if (color)
                s.push("<font color='" + color + "'>\u25A0</font> " + label);
            else
                s.push(label);
        }
        pageToolTip = s.join("<br>");
    }

    Fact {
        id: mPageName
        title: qsTr("Page name")
        descr: qsTr("Charts page name")
        flags: Fact.Text
        icon: "rename-box"
        onTextChanged: updateTitle()
    }
    Fact {
        id: mPinned
        title: qsTr("Pinned")
        descr: qsTr("Pin this page to the signals layout")
        flags: Fact.Bool
        icon: "pin"
        onValueChanged: updateDescr();
    }
    Fact {
        id: mSpeed
        title: qsTr("Speed")
        descr: qsTr("Charts speed")
        flags: Fact.Float
        icon: "speedometer"
        value: 1.0
        precision: 1
        min: 0.2
        max: 4
        onValueChanged: {
            if (active && setFact.active)
                sgMainChart.speedFactorValue = value;
            updateDescr();
        }
    }
    SignalsMenuChart {
        id: mMenuChart
        title: qsTr("Add new chart")
        descr: qsTr("Creating and setting a new chart")
        icon: "plus-circle"
        visible: !pageFact.newItem
        newItem: true
        onAddTriggered: createChart(save())
    }
    Fact {
        id: mCharts
        title: qsTr("Charts")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: {
            updateDescr();
            sgMainChart.resetEnable = true;
        }
    }

    // Actions
    Fact {
        id: pageAdd
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Add")
        enabled: newItem
        icon: "plus-circle"
        onTriggered: {
            pageFact.menuBack();
            addTriggered();
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        enabled: !newItem && changes
        icon: "check-circle"
        onTriggered: sgMenu.saveSettings()
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        visible: !newItem
        icon: "delete"
        onTriggered: {
            var active = pageFact.active;
            pageFact.deleteFact();
            if(active) setFact.setChecked(0);
        }
    }
}
