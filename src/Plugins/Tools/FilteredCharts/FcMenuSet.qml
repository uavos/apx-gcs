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

    property real speed: msSpeed.value

    property var values //from config
    property int editorsCnt: 1

    signal selected(var num)

    Fact {
        id: msTitle
        title: qsTr("Title")
        descr: qsTr("Charts title")
        flags: Fact.Text
        icon: "rename-box"
        value: chartFact.title
        onValueChanged: {
            fcBtn.toolTip = value;
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
            fcCharts.speed = value;
            fcCharts.speedFactorValue = value;
            console.log("Speed =", value);
        }
    }

    FcMenuChart {
        title: qsTr("Add new chart")
        descr: "Creating and setting a new chart"
        icon: "plus-circle"
        newItem: true
        // onAddTriggered: createNumber(save())
        onAddTriggered: createChart(save())
    }

    Fact {
        id: msValues
        title: qsTr("Values")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: updateBtnValues()
    }

    Component.onCompleted: updateSetItems()

    function updateBtnValues() {
        fcBtn.values = [];
        for (var i = 0; i < msValues.size; ++i) {
            var fact = msValues.child(i);
            fcBtn.values.push(fact);
        }
    }

    function save() {
        var values = [];
        for (var i = 0; i < msValues.size; ++i) {
            var mChart = msValues.child(i).save();
            if (!mChart.bind)
                continue;
            values.push(mChart);
        }
        if (!values)
            return;
        var set = {};
        set.title = title;
        set.values = values;
        return set;
    }

    function updateSetItems() {
        // msValues.onSizeChanged.disconnect(updateDescr);
        msValues.deleteChildren();
        for (var i in values) {
            createChart(values[i]);
        }
        // updateDescr();
        // msValues.onSizeChanged.connect(updateDescr);
    }

    function createChart(mChart) {
        if (!mChart.bind)
            return;
        if (mChart.bind === "")
            return;
        var c = createFact(msValues, "FcMenuChart.qml", {
            "data": mChart
        });
        // c.titleChanged.connect(updateDescr);
        // c.removeTriggered.connect(updateDescr);
    }

    // function updateDescr() {
    // if (!chartFact)
    //     return;
    // descr = "";
    // var s = [];
    // for (var i = 0; i < msValues.size; ++i) {
    //     s.push(msValues.child(i).title);
    // }
    // descr = s.join(',');
    // }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            return c;
        }
    }

    // Actions
    // Fact {
    //     flags: (Fact.Action | Fact.Remove)
    //     title: qsTr("Remove set")
    //     icon: "delete"
    //     onTriggered: {
    //         if (chartFact.active)
    //             select(0);
    //         chartFact.destroy();
    //     }
    // }

    // Fact {
    //     flags: (Fact.Action | Fact.Apply)
    //     title: qsTr("Select and save")
    //     visible: !chartFact.active
    //     icon: "check-circle"
    //     onTriggered: {
    //         chartFact.menuBack();
    //         chartFact.selected(chartFact.num);
    //     }
    // }
}
