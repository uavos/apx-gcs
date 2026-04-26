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

    function createDefaultSet() {
        var set = {
            "title": "default",
            "pages": [
                {
                    "title": "R",
                    "charts": [
                        {"bind": "cmd.att.roll", "color": "#ffffff"}, 
                        {"bind": "est.att.roll", "color": "#ffffff"}]
                },
                {
                    "title": "P",
                    "charts": [ 
                        {"bind": "cmd.att.pitch", "color": "#ffffff"},
                        {"bind": "est.att.pitch", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "Y",
                    "charts": [
                        {"bind": "cmd.pos.bearing", "color": "#ffffff"},
                        {"bind": "cmd.att.yaw", "color": "#ffffff"},
                        {"bind": "est.att.yaw", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "Axy",
                    "charts": [
                        {"bind": "est.acc.x", "color": "#ffffff"},
                        {"bind": "est.acc.y", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "Az",
                    "charts": [
                        {"bind": "est.acc.z", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "G",
                    "charts": [
                        {"bind": "est.gyro.x", "color": "#ffffff"},
                        {"bind": "est.gyro.y", "color": "#ffffff"},
                        {"bind": "est.gyro.z", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "Pt",
                    "charts": [
                        {"bind": "est.pos.altitude", "color": "#ffffff"},
                        {"bind": "est.pos.vspeed", "color": "#ffffff"},
                        {"bind": "est.air.airspeed", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "Ctr",
                    "charts": [
                        {"bind": "ctr.att.ail", "color": "#ffffff"},
                        {"bind": "ctr.att.elv", "color": "#ffffff"},
                        {"bind": "ctr.att.rud", "color": "#ffffff"},
                        {"bind": "ctr.eng.thr", "color": "#ffffff"},
                        {"bind": "ctr.eng.prop", "color": "#ffffff"},
                        {"bind": "ctr.str.rud", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "RC",
                    "charts": [
                        {"bind": "cmd.rc.roll", "color": "#ffffff"},
                        {"bind": "cmd.rc.pitch", "color": "#ffffff"},
                        {"bind": "cmd.rc.thr", "color": "#ffffff"},
                        {"bind": "cmd.rc.yaw", "color": "#ffffff"}
                    ]
                },
                {
                    "title": "Usr",
                    "charts": [
                        {"bind": "est.usr.u1", "color": "#ffffff"},
                        {"bind": "est.usr.u2", "color": "#ffffff"},
                        {"bind": "est.usr.u3", "color": "#ffffff"},
                        {"bind": "est.usr.u4", "color": "#ffffff"},
                        {"bind": "est.usr.u5", "color": "#ffffff"},
                        {"bind": "est.usr.u6", "color": "#ffffff"}
                    ]
                }
            ]
        };

        return set;
    }

    // function createDefaultSignals() {
        // return {
        //     "active": {
        //         "signals": 0
        //     },
        //     "sets": [createDefaultSet()]
        // };
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
                if (!(set.pages && (set.pages instanceof Array)))
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
        if (sets.length <= 0 || !json.active) {
            set = {};
            set = createDefaultSet();
            sets.push(set);
            currentSetIdx = sets.length - 1;
        }

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

    function resetToDefaults() {
        if(sgMenu.size <=0)
            return;
        var defaultSet = sgMenu.child(0);
        defaultSet.data = createDefaultSet();
        defaultSet.load(defaultSet.data);
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
        onTriggered: resetToDefaults()
    }
    Fact {
        title: qsTr("Save")
        flags: (Fact.Action | Fact.Apply)
        icon: "check-circle"
        onTriggered: saveSettings()
    }
}
