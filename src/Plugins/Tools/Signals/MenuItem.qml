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
    id: menuItem

    flags: Fact.Group
    precision: 2
    icon: "rectangle"

    property bool changes: false
    property bool newItem: false
    property var data: ({})

    // Runtime computed value (filtered)
    property var currentValue: undefined
    property string warningMsg: ""
    property real _warnTimestamp: 0

    // Cached compiled functions – rebuilt only when expressions change
    property var _bindFn: null
    property var _warnFn: null
    property var _alarmFn: null

    function _recompileBindFn() {
        var expr = mBind.text;
        if (!expr || expr === "") { _bindFn = null; return; }
        try {
            // eval creates a closure in QML scope, giving the compiled function
            // access to context properties (mandala, apx, Math, etc.).
            // Expressions are user-authored in signals.json — not external input.
            _bindFn = eval("(function() { return (" + expr + "); })");
        } catch(e) { _bindFn = null; }
    }
    function _recompileWarnFn() {
        var expr = mWarn.text;
        if (!expr || expr === "") { _warnFn = null; return; }
        try {
            _warnFn = new Function("value", "return !!(" + expr + ")");
        } catch(e) { _warnFn = null; }
    }
    function _recompileAlarmFn() {
        var expr = mAlarm.text;
        if (!expr || expr === "") { _alarmFn = null; return; }
        try {
            _alarmFn = new Function("value", "return !!(" + expr + ")");
        } catch(e) { _alarmFn = null; }
    }

    signal addTriggered
    signal removeTriggered

    // Warning/alarm state — propagated to PageButton
    property bool hasWarning: false
    property bool hasAlarm: false

    // Expose for PageButton tooltip
    property string itemTitle: mTitle.text ? mTitle.text : mBind.text
    property string itemColor: mColor.value ? mColor.value : "#ffffff"

    Component.onCompleted: {
        // Wire expression changes to recompile cached functions before loading
        // so the initial load triggers them automatically.
        mBind.textChanged.connect(_recompileBindFn);
        mWarn.textChanged.connect(_recompileWarnFn);
        mAlarm.textChanged.connect(_recompileAlarmFn);

        load(data);

        // Explicit recompile after load() in case textChanged did not fire
        // (e.g. the Fact text was already equal to the loaded value).
        _recompileBindFn();
        _recompileWarnFn();
        _recompileAlarmFn();

        updateTitle();
        updateDescr();
        mTitle.valueChanged.connect(updateTitle);
        mBind.valueChanged.connect(updateDescr);
        mColor.valueChanged.connect(function() { updateDescr(); setColor(); });
        mFilters.valueChanged.connect(updateDescr);
        mFact2Save.valueChanged.connect(updateDescr);
    }

    onCurrentValueChanged: saveValue2Fact()

    function load() {
        for (var i = 0; i < menuItem.size; ++i) {
            var f = child(i);
            var v = data[settingName(f)];
            if (v !== undefined)
                f.value = v;
        }
        mFilters.fillData();
        mColor.value = data.color ? data.color : "";
        changes = false;
        setColor();
    }

    function save() {
        data = {};
        for (var i = 0; i < menuItem.size; ++i) {
            var f = child(i);
            var s = f.text.trim();
            if (f.size !== 0)
                s = f.save();
            if (s === "")
                continue;
            data[settingName(f)] = s;
        }
        changes = false;
        setColor();
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
        var descrList = [];
        for (var i = 0; i < menuItem.size; ++i) {
            var f = child(i);
            if (!f.name)
                continue;
            if (f.name === "title")
                continue;
            if (f.text === "")
                continue;
            if (f.name === "color")
                descrList.push(f.name.toUpperCase() + ": <font color='" + f.text + "'>" + f.text.toUpperCase() + "</font>");
            else
                descrList.push(f.name.toUpperCase() + ": " + f.text);
        }
        descr = descrList.length > 0 ? descrList.join(", ") : "";
    }

    function setColor() {
        var opt = menuItem.opts;
        opt.color = mColor.value ? mColor.value : "#ffffff";
        opt.iconColor = opt.color;
        menuItem.opts = opt;
        mColor.changes = false;
        if (menuPage)
            menuPage.updatePageValues();
        if (signalsWidget)
            signalsWidget.updateSeriesColors();
    }

    // Telemetry update — computes filtered value and evaluates warn/alarm
    function updateValue() {
        if (!_bindFn) return;
        var v;
        try {
            v = _bindFn();
        } catch (e) {
            // Fallback: treat as a plain mandala fact path
            try {
                var _f = apx.fleet.current.mandala.fact(mBind.text, false);
                if (_f) v = _f.value;
            } catch (e2) {}
        }
        if (v === undefined || !isFinite(v))
            return;

        // Seed filter state on first valid value
        if (currentValue === undefined)
            mFilters.resetFilterState();

        // Run filter chain
        var filtered = mFilters.applyFilters(v);
        currentValue = filtered;
        menuItem.value = filtered;  // ChartsView reads fact.value

        // Evaluate warning/alarm using pre-compiled functions
        try { hasWarning = _warnFn ? _warnFn(filtered) : false; } catch(e) { hasWarning = false; }
        try { hasAlarm = _alarmFn ? _alarmFn(filtered) : false; } catch(e) { hasAlarm = false; }
    }

    function saveValue2Fact() {
        var fname = mFact2Save.text;
        if (!fname || fname === "")
            return;
        if (!apx.fleet.current.mandala.fact(fname, true))
            return;
        if (!fname.includes("sns.scr")) {
            emitWarning(qsTr("Unacceptable variable name. Use 'sns.scr' vars for saving!"));
            return;
        }
        apx.fleet.current.mandala.fact(fname, true).setRawValueLocal(currentValue);
    }

    function emitWarning(msg) {
        var now = Date.now();
        if (warningMsg === msg && (now - _warnTimestamp) < 10000)
            return;
        warningMsg = msg;
        _warnTimestamp = now;
        console.warn(qsTr("Chart") + " " + title + ": " + msg);
    }

    function hasScr(val) {
        if (!val || val !== mFact2Save.text)
            return false;
        emitWarning(val + " " + qsTr("variable already used"));
        return true;
    }

    Fact {
        id: mTitle
        name: "chartname"
        title: qsTr("Title")
        descr: qsTr("Chart name")
        flags: Fact.Text
        onTextChanged: changes = true
    }
    Fact {
        id: mBind
        name: "bind"
        title: qsTr("Expression")
        descr: "Math.atan(est.att.pitch/est.att.roll)"
        flags: Fact.Text
        onTextChanged: changes = true
    }
    MenuColor {
        id: mColor
        name: "color"
        title: qsTr("Color")
        descr: qsTr("Chart color")
    }
    MenuFilters {
        id: mFilters
        name: "filt"
        title: qsTr("Filters")
        descr: qsTr("Filter settings")
    }
    Fact {
        name: "warn"
        id: mWarn
        title: qsTr("Warning")
        descr: qsTr("Expression for warning (receives 'value')")
        flags: Fact.Text
        onValueChanged: changes = true
    }
    Fact {
        name: "alarm"
        id: mAlarm
        title: qsTr("Alarm")
        descr: "value>1.8 || (value>0 && value<1)"
        flags: Fact.Text
        onValueChanged: changes = true
    }
    Fact {
        name: "act"
        title: qsTr("Action")
        descr: "cmd.proc.action=proc_action_reset"
        flags: Fact.Text
        onValueChanged: changes = true
    }
    Fact {
        id: mFact2Save
        name: "save"
        title: qsTr("Save to")
        descr: qsTr("Variable for saving chart value (sns.scr.*)")
        flags: Fact.Int
        units: "mandala"
        onTextChanged: {
            signalsWidget.checkScrMatches(text);
            changes = true;
        }
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Add")
        enabled: newItem && mBind && mBind.value
        icon: "plus-circle"
        onTriggered: {
            menuItem.menuBack();
            addTriggered();
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        visible: !newItem
        icon: "delete"
        onTriggered: {
            removeTriggered();
            menuItem.deleteFact();
        }
    }
}
