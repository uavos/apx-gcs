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
    title: "Page #" + sgBtn.text

    property bool changes: false
    property alias speed: msSpeed.value
    property var values //from config

    Component.onDestruction: removed() // pinned menu closes when the plugin is closed

    function addNewChart() {
        mMenuChart.trigger();
    }

    function updateBtnValues() {
        sgBtn.values = [];
        for (var i = 0; i < mCharts.size; ++i)
            sgBtn.values.push(mCharts.child(i));
        sgBtn.updateToolTip(sgBtn.values);
    }

    function updateChartsValues() {
        for (var i = 0; i < mCharts.size; ++i)
            mCharts.child(i).updateValue();
    }

    function save() {
        changes = false;
        var tmpValues = [];
        for (var i = 0; i < mCharts.size; ++i) {
            var mchart = mCharts.child(i).save();
            if (!mchart.bind)
                continue;
            tmpValues.push(mchart);
        }
        var page = {};
        page.title = msTitle.value;
        page.speed = msSpeed.value;
        page.values = tmpValues;
        updateBtnValues();
        return page;
    }

    function load(page) {
        msTitle.value = page.title;
        msSpeed.value = page.speed;
        values = page.values;
        updateSetItems();
        changes = false;
    }

    function updateSetItems() {
        mCharts.deleteChildren();
        for (var i in values) {
            createChart(values[i]);
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
        c.removeTriggered.connect(function () {
            changes = true;
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

    function checkScrs(val) {
        var matches = false;
        for (var i = 0; i < mCharts.size; ++i)
            if (mCharts.child(i).hasScr(val))
                matches = true;
        return matches;
    }

    Fact {
        id: msTitle
        title: qsTr("Title")
        descr: qsTr("Charts title")
        flags: Fact.Text
        icon: "rename-box"
        value: pageFact.title
        onValueChanged: {
            pageFact.title = value;
            changes = true;
        }
    }
    Fact {
        id: msSpeed
        title: qsTr("Speed")
        descr: qsTr("Charts speed")
        flags: Fact.Float
        icon: "speedometer"
        value: 1.0
        precision: 1
        min: 0.2
        max: 4
        onValueChanged: {
            if (!sgBtn.checked)
                return;
            sgCharts.speedFactorValue = value;
            changes = true;
        }
    }
    SignalsMenuChart {
        id: mMenuChart
        title: qsTr("Add new chart")
        descr: qsTr("Creating and setting a new chart")
        icon: "plus-circle"
        newItem: true
        onAddTriggered: createChart(save())
    }
    Fact {
        id: mCharts
        title: qsTr("Charts")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: sgCharts.resetEnable = true
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        visible: changes
        icon: "check-circle"
        onTriggered: sgControl.saveSettings()
    }
}
