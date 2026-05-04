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
    id: fMenu

    flags: (Fact.Group | Fact.FlatModel)

    property bool changes: false
    property var data: ({})

    Component.onCompleted: load(value)
    onChangesChanged: { if (changes) mChart.changes = true;}

    function load(value) {
        if(!value)
            return;
        var filters = value.filters    
        if (filters === undefined || filters.length === 0)
            return;
        fSet.deleteChildren();
        for (var i in filters) {
            createFilter(filters[i]);
        }
        updateBtnValues();
        changes = false;
    }

    function save() {
        var tmpFilters = [];
        for (var i = 0; i < fSet.size; ++i) {
            var mfilter = fSet.child(i).save();
            tmpFilters.push(mfilter);
        }
        var fmenu = {};
        fmenu.filters = tmpFilters;
        changes = false;
        return fmenu;
    }

    function settingName(f) {
        var n = f.name;
        if (n.includes("_"))
            return n.slice(0, n.indexOf("_"));
        return n;
    }

    // Filters creation
    function createFilter(filterData) {
        var type =  filterData.type; 
        switch (type) {
            case "running_avg":
                createRunningAvg(filterData);
                break;
            case "kalman_smp":
                createKalmanSimple(filterData)
                break;
            default:
                console.warn(qsTr("Wrong filter type. Filter creation failed"))
        }
    }

    function createRunningAvg(filterData) {
        var c = createFact(fSet, "SignalsFilterRunningAvg.qml", {
            "data": filterData
        });
    }

    function createKalmanSimple(filterData) {
        var c = createFact(fSet, "SignalsFilterKalmanSimple.qml", {
            "data": filterData
        });
    }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            c.changed.connect(setChanges)
            return c;
        }
    }

    function setChanges(changesValue) {
        if(changesValue)
            fMenu.changes = changesValue;
    }

    // Using filters
    function useFilters(value, v) {
        for(var i = 0; i < fSet.size; ++i) {
            if(!fSet.child(i).value)
                continue;
            v = fSet.child(i).processValue(value, v)    
        }
        return v;
    }

    function setKalmanSimpleState(st, cv) {
        for(var i = 0; i < fSet.size; ++i)
            if(fSet.child(i).filterType === "kalman_smp")
                fSet.child(i).setKalmanState(st, cv);
    }

    SignalsFilterChooser {
        title: qsTr("Add new filter")
        descr: qsTr("Selecting and adding filters")
        icon: "plus-circle"
    }

    Fact {
        id: fSet
        title: qsTr("Filters")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: changes = true
    }

    // Actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Save")
        enabled: !mChart.newItem && changes
        icon: "check-circle"
        onTriggered: sgMenu.saveSettings()
    }
}
