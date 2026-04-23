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
        var f = application.prefs.loadFile("signals.json");
        var json = f ? JSON.parse(f) : {};

        // Legacy migration: {page, signalas} → {active, sets}
        if (json && json.signalas && !json.sets) {
            json = migrateLegacy(json);
        }

        var currentSetIdx = -1;

        if (json && json.sets) {
            for (var i in json.sets) {
                var set = json.sets[i];
                if (!set || !set.pages) continue;
                sets.push(set);
            }
            var setIdx = json.active ? json.active[settingsName] : 0;
            if (setIdx >= 0 && setIdx < sets.length)
                currentSetIdx = setIdx;
            else if (sets.length > 0)
                currentSetIdx = 0;
        }

        // First-run: generate default set
        if (sets.length <= 0 || !json.active) {
            var defSet = buildDefaultSet();
            sets.push(defSet);
            currentSetIdx = 0;
        }

        for (var i in sets) {
            var c = createSetFact(sets[i]);
            c.selected.connect(select);
            c.selected.connect(saveSettings);
        }
        select(currentSetIdx);
    }

    function saveSettings() {
        var fjson = application.prefs.loadFile("signals.json");
        var json = fjson ? JSON.parse(fjson) : {};
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
        application.prefs.saveFile("signals.json", JSON.stringify(json, ' ', 2));
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
            "title": setData.title ? setData.title : "set",
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
        var items = oldJson.signalas ? oldJson.signalas : [];
        var mappedItems = items.map(function(it) {
            return {
                bind: it.bind || it.name || "",
                title: it.title || "",
                color: it.color || "",
                filters: [],
                warn: it.warn || "",
                alarm: it.alarm || "",
                act: it.act || "",
                save: it.save || ""
            };
        }).filter(function(it) { return it.bind !== ""; });

        return {
            active: { signals: 0 },
            sets: [{
                title: "default",
                pages: [{
                    name: oldJson.page || "page 1",
                    pin: false,
                    speed: 1.0,
                    items: mappedItems
                }]
            }]
        };
    }

    // Default set matching the hardcoded Signals.qml pages
    function buildDefaultSet() {
        return {
            title: "default",
            pages: [
                {
                    name: "R",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.cmd.att.roll.value", title: "roll cmd" },
                        { bind: "mandala.est.att.roll.value", title: "roll" }
                    ]
                },
                {
                    name: "P",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.cmd.att.pitch.value", title: "pitch cmd" },
                        { bind: "mandala.est.att.pitch.value", title: "pitch" }
                    ]
                },
                {
                    name: "Y",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.cmd.pos.bearing.value", title: "bearing cmd" },
                        { bind: "mandala.cmd.att.yaw.value", title: "yaw cmd" },
                        { bind: "mandala.est.att.yaw.value", title: "yaw" }
                    ]
                },
                {
                    name: "Axy",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.est.acc.x.value", title: "Ax" },
                        { bind: "mandala.est.acc.y.value", title: "Ay" }
                    ]
                },
                {
                    name: "Az",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.est.acc.z.value", title: "Az" }
                    ]
                },
                {
                    name: "G",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.est.gyro.x.value", title: "Gx" },
                        { bind: "mandala.est.gyro.y.value", title: "Gy" },
                        { bind: "mandala.est.gyro.z.value", title: "Gz" }
                    ]
                },
                {
                    name: "Pt",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.est.pos.altitude.value", title: "alt" },
                        { bind: "mandala.est.pos.vspeed.value", title: "vspd" },
                        { bind: "mandala.est.air.airspeed.value", title: "airspeed" }
                    ]
                },
                {
                    name: "Ctr",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.ctr.att.ail.value", title: "ail" },
                        { bind: "mandala.ctr.att.elv.value", title: "elv" },
                        { bind: "mandala.ctr.att.rud.value", title: "rud" },
                        { bind: "mandala.ctr.eng.thr.value", title: "thr" },
                        { bind: "mandala.ctr.eng.prop.value", title: "prop" },
                        { bind: "mandala.ctr.str.rud.value", title: "str.rud" }
                    ]
                },
                {
                    name: "RC",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.cmd.rc.roll.value", title: "RC roll" },
                        { bind: "mandala.cmd.rc.pitch.value", title: "RC pitch" },
                        { bind: "mandala.cmd.rc.thr.value", title: "RC thr" },
                        { bind: "mandala.cmd.rc.yaw.value", title: "RC yaw" }
                    ]
                },
                {
                    name: "Usr",
                    pin: false,
                    speed: 1.0,
                    items: [
                        { bind: "mandala.est.usr.u1.value", title: "u1" },
                        { bind: "mandala.est.usr.u2.value", title: "u2" },
                        { bind: "mandala.est.usr.u3.value", title: "u3" },
                        { bind: "mandala.est.usr.u4.value", title: "u4" },
                        { bind: "mandala.est.usr.u5.value", title: "u5" },
                        { bind: "mandala.est.usr.u6.value", title: "u6" }
                    ]
                }
            ]
        };
    }

    // Actions
    Fact {
        title: qsTr("Add set")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: {
            var newSet = { title: "#" + (setsFact.size + 1), pages: [] };
            var c = createSetFact(newSet);
            c.selected.connect(select);
            c.selected.connect(saveSettings);
            c.trigger();
        }
    }

    Fact {
        title: qsTr("Save")
        flags: (Fact.Action | Fact.Apply)
        icon: "check-circle"
        onTriggered: saveSettings()
    }
}
