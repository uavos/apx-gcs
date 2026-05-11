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
    readonly property string configs: "signals_2.json"  // TODO: rename to "signals.json" after all tests  

    name: "signals"
    flags: (Fact.Group | Fact.DragChildren)
    title: qsTr("Signals")
    descr: qsTr("Realtime chart configuration editor")
    icon: "gauge"

    Component.onCompleted: loadSettings()
    Component.onDestruction: removed()

    function getCharts() {
        var charts = [];
        for (var i = 0; i < arguments.length; i++) {
            var fact = arguments[i];
            if (!fact)
                continue;
            if (!fact.opts)
                continue;
            if (!fact.opts.color)
                continue;
            var chart = {
                "bind": fact.mpath(),
                "color": fact.opts.color.toString()
            };
            charts.push(chart);
        }
        return charts;
    }

    function createDefaultSet() {
        var set = {
            "title": "default",
            "checked": 0,
            "pages": [
                {
                    "title": "R",
                    "charts": getCharts(mandala.cmd.att.roll, mandala.est.att.roll)
                },
                {
                    "title": "P",
                    "charts": getCharts(mandala.cmd.att.pitch, mandala.est.att.pitch)
                },
                {
                    "title": "Y",
                    "charts": getCharts(mandala.cmd.pos.bearing, mandala.cmd.att.yaw, mandala.est.att.yaw)
                },
                {
                    "title": "Axy",
                    "charts": getCharts(mandala.est.acc.x, mandala.est.acc.y)
                },
                {
                    "title": "Az",
                    "charts": getCharts(mandala.est.acc.z)
                },
                {
                    "title": "G",
                    "charts": getCharts(mandala.est.gyro.x, mandala.est.gyro.y, mandala.est.gyro.z)
                },
                {
                    "title": "Pt",
                    "charts": getCharts(mandala.est.pos.altitude, mandala.est.pos.vspeed, mandala.est.air.airspeed)
                },
                {
                    "title": "Ctr",
                    "charts": getCharts(mandala.ctr.att.ail, mandala.ctr.att.elv, mandala.ctr.att.rud, mandala.ctr.eng.thr, mandala.ctr.eng.prop, mandala.ctr.str.rud)
                },
                {
                    "title": "RC",
                    "charts": getCharts(mandala.cmd.rc.roll, mandala.cmd.rc.pitch, mandala.cmd.rc.thr, mandala.cmd.rc.yaw)
                },
                {
                    "title": "Usr",
                    "charts": getCharts(mandala.est.usr.u1, mandala.est.usr.u2, mandala.est.usr.u3, mandala.est.usr.u4, mandala.est.usr.u5, mandala.est.usr.u6)
                }
            ]
        };
        return set;
    }

    function loadSettings() {
        var sets = [];
        var f = application.prefs.loadFile(configs);
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
        var fjson = application.prefs.loadFile(configs);
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
        application.prefs.saveFile(configs, JSON.stringify(json, ' ', 2));
        // console.log("Signals settings saved");
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
        var setIndex = -1
        var set = createDefaultSet();
        for(var i = 0; i < sgMenu.size; ++ i) {
            var title = sgMenu.child(i).title.trim().toLowerCase()
            if (title === "default") {
                var defaultSet = sgMenu.child(i);
                defaultSet.load(set);
                defaultSet.move(0)
                setIndex = i;
                break;
           }
        }
        if (setIndex < 0) {
            var c = createFact(sgMenu, "SignalsMenuSet.qml", {
                "data": set
            });
            c.selected.connect(select);
            c.selected.connect(saveSettings);
            c.move(0);
        }
        select(0);
    }

    function getActiveSetIndex() {
        for (var i = 0; i < sgMenu.size; ++i) {
            if (sgMenu.child(i).active)
                return i;
        }
        return sgMenu.size > 0 ? 0 : -1;
    }

    function getActiveSet() {
        var index = getActiveSetIndex();
        if (index < 0 || index >= sgMenu.size)
            return null;
        return sgMenu.child(index);
    }

    function getActivePages() {
        if (sgMenu.size <= 0)
            return [];
        var activeSet = getActiveSet();
        if (!activeSet)
            return [];
        return activeSet.getPages();
    }

    function getActivePage() {
        if (sgMenu.size <= 0)
            return null;
        var activeSet = getActiveSet();
        if (!activeSet)
            return null;
        return activeSet.getActivePage();
    }

    function getPinnedPages() {
        if (sgMenu.size <= 0)
            return [];
        var activeSet = getActiveSet();
        if (!activeSet)
            return [];
        return activeSet.getPinned();
    }

    Fact {
        title: qsTr("Add set")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: createSet()
    }
    Fact {
        title: qsTr("Reset to defaults")
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
