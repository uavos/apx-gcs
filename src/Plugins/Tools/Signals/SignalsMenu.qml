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

// Top-level sets editor — mirrors NumbersMenu.
// Loaded as a pinned Fact menu popup.
// Persists to signals.json using same {active, sets} structure as numbers.json.
Fact {
    id: setsFact

    readonly property QtObject signalsModel: SignalsModel {}

    property var defaults
    property string settingsName: "signals"
    property bool destroyOnClose: true

    name: settingsName
    flags: (Fact.Group | Fact.DragChildren)
    title: qsTr("Signals") + ": " + settingsName
    descr: qsTr("Signals chart sets editor")
    icon: "poll"

    signal accepted()

    Component.onCompleted: open()

    function open() {
        if (!parentFact) {
            var p = parent;
            parentFact = apx.fleet.local;
            parent = p;
        }
        loadSettings();
    }

    function close() {
        if (!destroyOnClose) {
            setsFact.deleteChildren();
            loadSettings();
            menuBack();
            return;
        }
        setsFact.deleteChildren();
        menuBack();
        parentFact = null;
    }

    function loadSettings() {
        var sets = [];
        var json = signalsModel.loadJson();

        var currentSetIdx = -1;

        if (json && json.sets) {
            for (var i in json.sets) {
                var set = json.sets[i];
                if (!set || !set.pages) continue;
                sets.push(set);
            }
        }
        currentSetIdx = signalsModel.activeIndex(json);

        for (var i in sets) {
            var c = createSetFact(sets[i]);
            if (!c) continue;
            c.selected.connect(select);
            c.selected.connect(saveSettings);
        }
        select(currentSetIdx);
    }

    function saveSettings() {
        var json = signalsModel.loadJson();
        if (!json.active)
            json.active = {};
        json.active[settingsName] = 0;
        json.sets = [];
        for (var i = 0; i < size; ++i) {
            var setf = child(i);
            var set = setf.save();
            if (!set) continue;
            json.sets.push(set);
            if (setf.active)
                json.active[settingsName] = i;
        }
        signalsModel.saveJson(json);
        accepted();
        close();
    }

    function createSetFact(setData) {
        var component = Qt.createComponent("MenuSet.qml");
        if (component.status !== Component.Ready) {
            console.warn("SignalsMenu: cannot load MenuSet.qml: " + component.errorString());
            return null;
        }
        var c = component.createObject(setsFact, {
            "title": setData.title ? setData.title : qsTr("set"),
            "pages": setData.pages ? setData.pages : []
        });
        c.parentFact = setsFact;
        return c;
    }

    function select(num) {
        for (var i = 0; i < setsFact.size; ++i) {
            var set = setsFact.child(i);
            set.active = (set.num === num);
        }
    }

    // Legacy migration: old flat {page, signalas} → {active, sets}
    function migrateLegacy(oldJson) {
        return signalsModel.migrateLegacy(oldJson);
    }

    // Default set matching the hardcoded Signals.qml pages
    function buildDefaultSet() {
        return signalsModel.buildDefaultSet();
    }

    // Actions
    Fact {
        title: qsTr("Add set")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: {
            var newSet = { title: "#" + (setsFact.size + 1), pages: [] };
            var c = createSetFact(newSet);
            if (!c) return;
            c.selected.connect(select);
            c.selected.connect(saveSettings);
            c.trigger();
        }
    }

    Fact {
        title: qsTr("Reset to defaults")
        flags: Fact.Action
        icon: "restore"
        onTriggered: {
            var defaultSet = buildDefaultSet();
            var defaultFact = setsFact.size > 0 ? setsFact.child(0) : null;
            if (!defaultFact) {
                defaultFact = createSetFact(defaultSet);
                if (!defaultFact)
                    return;
                defaultFact.selected.connect(select);
                defaultFact.selected.connect(saveSettings);
            } else {
                defaultFact.loadSet(defaultSet);
            }
            select(0);
        }
    }

    Fact {
        title: qsTr("Save")
        flags: (Fact.Action | Fact.Apply)
        icon: "check-circle"
        onTriggered: saveSettings()
    }
}
