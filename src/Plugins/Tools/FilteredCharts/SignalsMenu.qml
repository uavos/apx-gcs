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
    id: sgMenu
    property var defaults
    property bool destroyOnClose: true

    name: "signals"
    flags: (Fact.Group | Fact.DragChildren)
    title: qsTr("Signals")
    descr: qsTr("Realtime chart configuration editor")
    icon: "gauge"

    // signal accepted

    // Component.onCompleted: open()

    Component.onCompleted: loadSettings()

    // function open() {
    //     //ensure mandala linked to unit
    //     if (!parentFact) {
    //         var p = parent;
    //         parentFact = apx.fleet.local;
    //         parent = p;
    //     }
    //     loadSettings();
    // }

    // function close() {
    //     if (!destroyOnClose) {
    //         sgMenu.deleteChildren();
    //         loadSettings();
    //         menuBack();
    //         return;
    //     }
    //     sgMenu.deleteChildren();
    //     menuBack();
    //     parentFact = null;
    // }

    function loadSettings() {
        var sets = [];
        var f = application.prefs.loadFile("signals_2.json");
        var json = f ? JSON.parse(f) : {};
        var set = {};
        var currentSetIdx = -1;
        if (json && json.sets) {
            for (var i in json.sets) {
                set = json.sets[i];
                if (!(set.values && (set.values instanceof Array)))
                    continue;
                sets.push(set);
            }
            //set index
            var setIdx = json.active[name];
            if (setIdx >= 0 && setIdx < sets.length)
                currentSetIdx = setIdx;
            else if (sets.length > 0)
                currentSetIdx = 0;
        }
        //defaults
        // if (sets.length <= 0 || !json.active) {
        // createDefault();
        // set = {};
        // set.title = settingsName;
        // set.values = defaults;
        // sets.push(set);
        // currentSetIdx = sets.length - 1;
        // }

        //create facts
        for (i in sets) {
            var c = createFact(sgMenu, "SignalsMenuSet.qml", {
                "data": sets[i]
            });
            c.selected.connect(select);
            c.selected.connect(saveSettings);
        }
        select(currentSetIdx);
    }

    function saveSettings() {
        var fjson = application.prefs.loadFile("signals_2.json");
        var json = fjson ? JSON.parse(fjson) : {};
        if (!json.active)
            json.active = {};
        json.active[name] = 0;
        json.sets = [];
        for (var i = 0; i < size; ++i) {
            var setFact = child(i);
            var set = setFact.save();
            if (!set)
                continue;
            json.sets.push(set);
            if (setFact.active)
                json.active[name] = i;
        }
        application.prefs.saveFile("signals_2.json", JSON.stringify(json, ' ', 2));
        // accepted();
        // close();
    }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            return c;
        }
    }

    function select(num) {
        for (var i = 0; i < sgMenu.size; ++i) {
            var set = sgMenu.child(i);
            set.active = set.num == num;
        }
    }

    function createSet() {
        var set = {};
        set.title = "Set" + (sgMenu.size + 1);
        set.values = [];
        var c = createFact(sgMenu, "SignalsMenuSet.qml", {
            "data": set
        });
        c.selected.connect(select);
        c.selected.connect(saveSettings);
        c.trigger();
    }

    Fact {
        title: qsTr("Add set")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: createSet()
    }
    Fact {
        title: qsTr("Reset defaults")
        flags: Fact.Action
        icon: "restore"
        // onTriggered: sgMenu.resetToDefaults()
        onTriggered: console.log("Not implemented")
    }
    Fact {
        title: qsTr("Save")
        flags: (Fact.Action | Fact.Apply)
        icon: "check-circle"
        onTriggered: saveSettings()
    }
}
