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
    id: filterItem

    flags: (Fact.Group | Fact.FlatModel)

    property var data: ({})
    property bool changes: false
    property var paramsFact: null

    signal removeTriggered

    readonly property var registry: ({
        "running_avg": {
            title: qsTr("Running average"),
            url: "FilterRunningAvg.qml"
        },
        "kalman_smp": {
            title: qsTr("Kalman simple"),
            url: "FilterKalmanSimple.qml"
        }
    })

    Component.onCompleted: {
        var opt = opts;
        opt.page = "qrc:/Signals/MenuFilterItemPage.qml";
        opts = opt;
        load(data);
    }

    function filterTitle(typeName) {
        var meta = registry[typeName];
        return meta ? meta.title : typeName;
    }

    function createParamsFact(typeName) {
        if (paramsFact)
            paramsFact.deleteFact();

        var meta = registry[typeName] || registry.running_avg;
        var component = Qt.createComponent(meta.url);
        if (component.status !== Component.Ready) {
            console.warn("FilterItem: cannot load " + meta.url + ": " + component.errorString());
            paramsFact = null;
            return null;
        }

        paramsFact = component.createObject(filterParams, {});
        if (!paramsFact) {
            console.warn("FilterItem: failed to create params fact for " + typeName);
            return null;
        }
        paramsFact.parentFact = filterParams;
        paramsFact.title = meta.title;
        paramsFact.descr = meta.title;
        updateDescr();
        return paramsFact;
    }

    function updateDescr() {
        var parts = [];
        parts.push(filterEnabled.value > 0 ? qsTr("enabled") : qsTr("disabled"));
        if (paramsFact && paramsFact.text)
            parts.push(paramsFact.text);
        descr = parts.join(", ");
        title = filterTitle(filterType.text);
    }

    function load(filterData) {
        data = filterData || {};

        var typeName = data.type ? data.type : "running_avg";
        var typeIndex = filterType.enumStrings.indexOf(typeName);
        filterType.value = typeIndex >= 0 ? typeIndex : 0;
        filterEnabled.value = data.enabled === undefined ? true : !!data.enabled;

        createParamsFact(filterType.text);
        if (paramsFact && typeof paramsFact.loadFromObject === "function")
            paramsFact.loadFromObject(data);

        changes = false;
        updateDescr();
    }

    function save() {
        var result = {
            type: filterType.text,
            enabled: filterEnabled.value > 0
        };

        if (paramsFact && typeof paramsFact.save === "function") {
            var params = paramsFact.save();
            for (var key in params)
                result[key] = params[key];
        }

        data = result;
        changes = false;
        updateDescr();
        return result;
    }

    function applyFilter(input) {
        if (!(filterEnabled.value > 0) || !paramsFact || typeof paramsFact.filterValue !== "function")
            return input;
        return paramsFact.filterValue(input);
    }

    function resetState() {
        if (paramsFact && typeof paramsFact.resetState === "function")
            paramsFact.resetState();
    }

    Fact {
        id: filterType
        name: "type"
        title: qsTr("Type")
        descr: qsTr("Filter type")
        flags: Fact.Enum
        enumStrings: ["running_avg", "kalman_smp"]
        onValueChanged: {
            createParamsFact(text);
            changes = true;
            updateDescr();
        }
    }

    Fact {
        id: filterEnabled
        name: "enabled"
        title: qsTr("Enabled")
        descr: qsTr("Enable this filter in the chain")
        flags: Fact.Bool
        value: true
        onValueChanged: {
            changes = true;
            updateDescr();
        }
    }

    Fact {
        id: filterParams
        title: qsTr("Parameters")
        flags: (Fact.Group | Fact.Section)
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove filter")
        icon: "delete"
        onTriggered: {
            removeTriggered();
            filterItem.deleteFact();
        }
    }
}
