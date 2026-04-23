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

// Ordered filter chain for a single chart item.
// To add a new filter type:
//   1. Create FilterMyType.qml with loadFromObject()/save()/filterValue()/resetState()
//   2. Register the type in FilterItem.qml registry and enumStrings
//   3. The chain will pick it up automatically
Fact {
    id: menuFilters

    property bool changes: false
    property var data: []

    property var newFilterTypeFact: newFilterType
    property var addFilterFact: addFilterAction
    property var filtersFact: filterValues

    Component.onCompleted: {
        var opt = opts;
        opt.page = "qrc:/Signals/MenuFiltersPage.qml";
        opts = opt;
    }

    onChangesChanged: { if (changes) menuItem.changes = true; }

    function createFilter(filterData) {
        var component = Qt.createComponent("FilterItem.qml");
        if (component.status !== Component.Ready) {
            console.warn("MenuFilters: cannot load FilterItem.qml: " + component.errorString());
            return null;
        }

        var filterFact = component.createObject(filterValues, {
            "data": filterData || {
                type: "running_avg",
                enabled: true
            }
        });
        if (!filterFact) {
            console.warn("MenuFilters: failed to create FilterItem instance");
            return null;
        }
        filterFact.parentFact = filterValues;
        filterFact.removeTriggered.connect(function() {
            updateDescr();
            changes = true;
        });
        filterFact.titleChanged.connect(updateDescr);
        updateDescr();
        return filterFact;
    }

    function resetFilterState() {
        for (var i = 0; i < filterValues.size; ++i)
            filterValues.child(i).resetState();
    }

    function applyFilters(v) {
        var result = v;
        for (var i = 0; i < filterValues.size; ++i)
            result = filterValues.child(i).applyFilter(result);
        return result;
    }

    function normalizeData(rawData) {
        if (Array.isArray(rawData))
            return rawData;

        if (rawData && typeof rawData === "object") {
            if (rawData.type)
                return [rawData];

            if (rawData.filters && Array.isArray(rawData.filters))
                return rawData.filters;

            // Legacy single-selector shape
            var legacyType = rawData.filters || rawData.filterType;
            if (legacyType === "running_avg")
                return [{
                    type: "running_avg",
                    enabled: true,
                    coef: rawData.running_avg || rawData.coef || rawData.coefficient || 0.5
                }];
            if (legacyType === "kalman_smp")
                return [{
                    type: "kalman_smp",
                    enabled: true,
                    r: rawData.measurement_noise !== undefined ? rawData.measurement_noise : (rawData.r !== undefined ? rawData.r : 1),
                    q: rawData.environment_noise !== undefined ? rawData.environment_noise : (rawData.q !== undefined ? rawData.q : 1)
                }];
        }

        return [];
    }

    function load() {
        filterValues.deleteChildren();
        var list = normalizeData(data);
        for (var i = 0; i < list.length; ++i)
            createFilter(list[i]);
        menuFilters.value = list;
        changes = false;
        updateDescr();
    }

    function save() {
        data = [];
        for (var i = 0; i < filterValues.size; ++i)
            data.push(filterValues.child(i).save());
        menuFilters.value = data;
        changes = false;
        updateDescr();
        return data;
    }

    function updateDescr() {
        var labels = [];
        for (var i = 0; i < filterValues.size; ++i)
            labels.push(filterValues.child(i).title);
        descr = labels.length > 0 ? labels.join(", ") : qsTr("No filters");
    }

    function fillData() {
        if (value !== undefined && value !== null) {
            data = value;
            load();
        }
    }

    Fact {
        id: newFilterType
        name: "new_filter_type"
        title: qsTr("New filter type")
        descr: qsTr("Filter type to append to the chain")
        flags: Fact.Enum
        enumStrings: ["running_avg", "kalman_smp"]
    }

    Fact {
        id: addFilterAction
        title: qsTr("Add filter")
        descr: qsTr("Append a new filter to the chain")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: {
            createFilter({
                type: newFilterType.text,
                enabled: true
            });
            changes = true;
        }
    }

    Fact {
        id: filterValues
        title: qsTr("Filters")
        descr: qsTr("Ordered filter chain")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
        onSizeChanged: {
            updateDescr();
            changes = true;
        }
        onItemMoved: {
            updateDescr();
            changes = true;
        }
    }
}

