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
import Apx.Common

Fact {
    id: mChart

    flags: Fact.Group
    precision: 2
    icon: "rectangle"

    property bool changes: false
    property bool newItem: false
    property bool warning: false
    property var warningMsg: ""
    property var data: ({})

    // Chart values
    property var type: "none"
    property var expr: ""
    property var exprW: ""
    property var scr: ""

    signal addTriggered

    Component.onCompleted: {
        load(data);
        updateTitle();
        updateDescr();
        mBind.valueChanged.connect(updateTitle);
        mBind.valueChanged.connect(updateDescr);
        mColor.valueChanged.connect(updateDescr);
        mFilters.descrChanged.connect(updateDescr);
        mWarn.textChanged.connect(updateDescr);
        mFact2Save.valueChanged.connect(updateDescr);
    }

    onValueChanged: saveValue2Fact()
    onChangesChanged: {if (changes && !newItem) pageFact.changes = true}

    function load(data) {
        for (var i = 0; i < mChart.size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            f.value = v;
        }
        updateChartVars();
    }

    function save() {
        data = {};
        for (var i = 0; i < mChart.size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (f.size != 0 && f.name !== "color") {
                s = f.save();
            }
            if (s === "") {
                continue;
            }
            data[settingName(f)] = s;
        }
        updateChartVars();
        return data;
    }

    function settingName(f) {
        var n = f.name;
        if (n.includes("_"))
            return n.slice(0, n.indexOf("_"));
        return n;
    }

    function updateChartVars() {
        expr = mBind.text;
        exprW = mWarn.text;
        type = mFilters.value;
        scr = mFact2Save.text;
        if (type === "kalman_smp") {
            var v = value !== undefined ? value : 0;
            mFilters.setKalmanSimpleState(v, 0.1); // set start state and covariance
        }
        changes = false;
        setColor();
    }

    function updateTitle() {
        if (newItem)
            return;
        if (mBind.text != "")
            title = mBind.text;
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
            if (f.text === "")
                continue;
            if (f.name === "filt") {
                var usedFilters = f.getUsedFilterNames();
                if (usedFilters.length > 0) {
                    descrList.push(f.name.toUpperCase() + ": " + f.getUsedFilterNames().join(", "));
                } 
            } else if (f.name === "color") {
                descrList.push(f.name.toUpperCase() + ": " + f.text.toUpperCase()); 
            } else {
                descrList.push(f.name.toUpperCase() + ": " + f.text);
            }
        }
        if (descrList.length > 0)
            descr = descrList.join(", ");
        else
            descr = "";
    }

    function setColor() {
        var opt = mChart.opts;
        opt.color = mColor.text ? mColor.text : "#ffffff"; // Black color for chart turn into white
        mChart.opts = opt;
        if (!newItem)
            updateIconColor(opt.color)
        mColor.changes = false;
        pageFact.updateBtnValues();
    }

    function updateIconColor(iconColor) {
        var opt = mChart.opts;
        opt.iconColor = iconColor;
        mChart.opts = opt;
    }

    function updateValue() {
        try {
            var v = new Function('return ' + expr)();
            if (v === undefined)
                throw new Error(qsTr("expression is undefined"));
            // For first init
            if (value === undefined) {
                mFilters.setKalmanSimpleState(v, 0.1);
                value = v;
                return;
            }
            // Use filters
            value = mFilters.useFilters(value, v)
        } catch (e) {
            chartWarning(e.message);
        }
    }

    function updateWarning () {
        try {
            if(exprW.trim() === "") {
                warning = false;
                return;
            }
            var v = new Function('return ' + exprW)();
            if (v === undefined)
                throw new Error(qsTr("expression is undefined"));
            if(typeof v !== 'boolean')
                throw new Error(qsTr("wrong warning condition (not true/false)"));
            warning = v;
        } catch (e) {
            chartWarning(e.message);
        }
    }

    function saveValue2Fact() {
        var fname = scr;
        if (!fname || fname === "" || fname === undefined)
            return;
        if (!apx.fleet.current.mandala.fact(fname, true))
            return;
        if (!fname.includes("sns.scr")) {
            chartWarning(qsTr("Unacceptable variable name. Use 'sns.scr' vars for saving!"));
            return;
        }
        apx.fleet.current.mandala.fact(fname, true).setRawValueLocal(value);
    }

    function chartWarning(msg) {
        if (warningMsg == msg && warnTimer.running)
            return;
        warningMsg = msg;
        console.warn(qsTr("Chart") + " " + title + ": " + msg);
        warnTimer.restart();
    }

    function hasScr(val) {
        if (!val || val !== scr)
            return;
        chartWarning(val + " " + qsTr("variable already used"));
    }

    Fact {
        id: mFact
        title: qsTr("Binding")
        descr: qsTr("Fact value")
        flags: Fact.Int
        units: "mandala"
        onTextChanged: if (value) mBind.setValue(text)
    }
    Fact {
        id: mBind
        name: "bind"
        title: qsTr("Expression")
        descr: "Math.atan(est.att.pitch/est.att.roll)"
        flags: Fact.Text
        onTextChanged: changes = true;
    }
    SignalsMenuColor {
        id: mColor
        name: "color"
        title: qsTr("Color")
        descr: qsTr("Chart color")
        opts: ({
            "editor": Qt.resolvedUrl("SignalsColorEditor.qml")
        })
        value: "#ffffff"
    }
    SignalsMenuFilters {
        id: mFilters
        name: "filt"
        title: qsTr("Filters")
        descr: qsTr("Filters settings")
        onDescrChanged: changes = true
    }
    Fact {
        id: mWarn
        name: "warn"
        title: qsTr("Warning")
        descr: qsTr("Expression for warning")
        flags: Fact.Text
        onTextChanged: changes = true;
    }
    Fact {
        id: mFact2Save
        name: "save"
        title: qsTr("Save to")
        descr: qsTr("Variable for saving chart value")
        flags: Fact.Int
        units: "mandala"
        onTextChanged: {
            setFact.checkScrMatches(text);
            changes = true;
        }
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
        enabled: !newItem && !mAdd.enabled && changes
        icon: "check-circle"
        onTriggered: sgMenu.saveSettings()
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        visible: !newItem
        icon: "delete"
        onTriggered: mChart.deleteFact();
    }
}
