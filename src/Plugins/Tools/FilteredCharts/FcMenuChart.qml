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
    // id: number
    id: mChart
    flags: Fact.Group
    precision: 2

    property bool changed: false
    property bool newItem: false
    property var data: ({})

    signal addTriggered
    signal removeTriggered

    Component.onCompleted: {
        load(data);
        updateTitle();
        updateDescr();
        setColor();
        mTitle.valueChanged.connect(updateTitle);
        mBind.valueChanged.connect(updateDescr);
        mColor.valueChanged.connect(updateDescr);
    }

    function updateFilters() {
        mFilters.fillFilters();
    }

    function load() {
        for (var i = 0; i < mChart.size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            f.value = v;
        }
        setChanged(false);
    }

    function save() {
        data = {};
        for (var i = 0; i < mChart.size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (f.size != 0) {
                s = f.save();
            }
            if (s === "") {
                continue;
            }
            data[settingName(f)] = s;
        }
        return data;
    }

    function settingName(f) {
        var n = f.name;
        if (n.includes("_"))
            return n.slice(0, n.indexOf("_"));
        return n;
    }

    function updateTitle() {
        if (newItem)
            return;
        title = mTitle.text ? mTitle.text : mBind.text;
    }

    function updateDescr() {
        if (newItem)
            return;

        // List non-zero values in descr
        var descrList = [];
        for (var i = 0; i < mChart.size; ++i) {
            var f = child(i);
            if (!f.name)
                continue;
            if (f.name === "title")
                continue;
            if (f.text === "")
                continue;
            descrList.push(f.name.toUpperCase() + ": " + f.text);
        }
        if (descrList.length > 0)
            descr = descrList.join(", ");
        else
            descr = "";
    }

    function setColor() {
        var opt = mChart.opts;
        opt.color = mColor.text;
        mChart.opts = opt;
    }

    function updateValue() {
        var exp = mBind.text;
        if (eval(exp) == undefined)
            return;
        // Use filters
        var v = eval(exp);
        var type = mFilters.value;
        switch (type) {
        case "running_avg":
            useRunningAvgFilter(v);
        default:
            value = v;
        }
    }

    function setChanged(v) {
        if (newItem)
            return;
        changed = v;
    }

    // Filters functions
    function useRunningAvgFilter(v) {
        var k = mFilters.getRunningAvgCoef();
        value += (v - value) * k;

        // console.log("use running avg filter", v, "/", value);
    }

    Fact {
        id: mTitle
        name: "title"
        title: qsTr("Title")
        descr: qsTr("Chart name")
        flags: Fact.Text
        onTextChanged: setChanged(true)
    }
    Fact {
        id: mBind
        name: "bind"
        title: qsTr("Expression")
        descr: "Math.atan(est.att.pitch/est.att.roll)"
        flags: Fact.Text
        onTextChanged: setChanged(true)
    }
    Fact {
        id: mColor
        name: "color"
        title: qsTr("Color")
        descr: qsTr("Chart color")
        flags: Fact.Enum
        enumStrings: ["red", "orange", "yellow", "green", "aqua", "blue", "purple", "violet"]
        onTextChanged: setColor()
        onValueChanged: setChanged(true)
    }
    FcFiltersMenu {
        id: mFilters
        name: "filt"
        title: qsTr("Filters")
        descr: qsTr("Filters settings")
    }
    Fact {
        name: "warn"
        title: qsTr("Warning")
        descr: qsTr("Expression for warning")
        flags: Fact.Text
        enabled: false
        visible: false
    }
    Fact {
        name: "alarm"
        title: qsTr("Alarm")
        descr: "value>1.8 || (value>0 && value<1)"
        flags: Fact.Text
        enabled: false
        visible: false
    }
    Fact {
        name: "act"
        title: qsTr("Action")
        descr: "cmd.proc.action=proc_action_reset"
        flags: Fact.Text
        enabled: false
        visible: false
    }

    // Actions
    Fact {
        id: mAdd
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Add")
        enabled: newItem && mBind && mBind.value
        icon: "plus-circle"
        onTriggered: {
            mChart.menuBack();
            addTriggered();
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        enabled: changed && !newItem && !mAdd.enable
        icon: "plus-circle"
        onTriggered: {
            fcControl.saveSettings();
            setChanged(false);
        }
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        visible: !newItem
        icon: "delete"
        onTriggered: {
            removeTriggered();
            mChart.deleteFact();
        }
    }
}
