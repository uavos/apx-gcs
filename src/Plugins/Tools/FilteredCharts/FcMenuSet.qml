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
    id: chartFact
    
    flags: (Fact.Group | Fact.FlatModel)
    title: "Charts #" + fcBtn.text

    property bool changes: false
    property alias speed: msSpeed.value
    property var values //from config

    function addNewChart() {
        msMenuChart.trigger();
    }

    function updateBtnValues() {
        fcBtn.values = [];
        for (var i = 0; i < msValues.size; ++i)
            fcBtn.values.push(msValues.child(i));
        fcBtn.updateToolTip(fcBtn.values);   
    }

    function updateChartsValues() {
        for (var i = 0; i < msValues.size; ++i)
            msValues.child(i).updateValue();
    }

    function save() {
        changes = false;
        var tmpValues = [];
        for (var i = 0; i < msValues.size; ++i) {
            var mchart = msValues.child(i).save();
            if (!mchart.bind)
                continue;
            tmpValues.push(mchart);
        }
        var set = {};
        set.title = msTitle.value;
        set.speed = msSpeed.value;
        set.values = tmpValues;
        updateBtnValues()
        return set;
    }

    function load(set) {
        msTitle.value = set.title;
        msSpeed.value = set.speed;
        values = set.values;
        updateSetItems();
        changes = false;
    }

    function updateSetItems() {
        msValues.deleteChildren();
        for (var i in values) {
            createChart(values[i]);
        }
        updateBtnValues()
    }

    function createChart(mchart) {
        if (!mchart.bind)
            return;
        if (mchart.bind === "")
            return;
        var c = createFact(msValues, "FcMenuChart.qml", {
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

    Fact {
        id: msTitle
        title: qsTr("Title")
        descr: qsTr("Charts title")
        flags: Fact.Text
        icon: "rename-box"
        value: chartFact.title
        onValueChanged: {
            chartFact.title = value;
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
            if(!fcBtn.checked)
                return;
            fcCharts.speedFactorValue = value;
            changes = true;
        }
    }
    FcMenuChart {
        id: msMenuChart
        title: qsTr("Add new chart")
        descr: "Creating and setting a new chart"
        icon: "plus-circle"
        newItem: true
        onAddTriggered: createChart(save())
    }
    Fact {
        id: msValues
        title: qsTr("Values")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: fcCharts.resetEnable = true;
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        visible: changes
        icon: "check-circle"
        onTriggered: fcControl.saveSettings()
    }
}
