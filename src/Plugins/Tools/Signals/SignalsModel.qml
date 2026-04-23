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

QtObject {
    id: signalsModel

    property string settingsName: "signals"
    property string fileName: "signals.json"

    function loadRawJson() {
        var fileData = application.prefs.loadFile(fileName);
        return fileData ? JSON.parse(fileData) : {};
    }

    function saveJson(json) {
        application.prefs.saveFile(fileName, JSON.stringify(json, " ", 2));
    }

    function clone(value) {
        return JSON.parse(JSON.stringify(value));
    }

    function normalizeJson(json) {
        var normalized = json && typeof json === "object" ? clone(json) : {};

        if (normalized.signalas && !normalized.sets)
            normalized = migrateLegacy(normalized);

        if (!normalized.active)
            normalized.active = {};

        var sets = Array.isArray(normalized.sets) ? normalized.sets.filter(function(setData) {
            return !!setData;
        }) : [];
        if (sets.length === 0)
            sets = [buildDefaultSet()];

        normalized.sets = sets;

        var idx = normalized.active[settingsName];
        if (idx === undefined || idx < 0 || idx >= sets.length)
            normalized.active[settingsName] = 0;

        return normalized;
    }

    function loadJson() {
        return normalizeJson(loadRawJson());
    }

    function activeIndex(json) {
        var normalized = normalizeJson(json);
        return normalized.active[settingsName];
    }

    function activeSet(json) {
        var normalized = normalizeJson(json);
        return normalized.sets[normalized.active[settingsName]];
    }

    function migrateLegacy(oldJson) {
        var items = (oldJson.signalas || [])
            .map(function(it) {
                return {
                    bind: it.bind || it.name || "",
                    title: it.title || "",
                    color: it.color || "",
                    filters: [],
                    warning: it.warning || it.warn || "",
                    alarm: it.alarm || "",
                    act: it.act || "",
                    save: it.save || ""
                };
            })
            .filter(function(it) { return it.bind !== ""; });

        return {
            active: { signals: 0 },
            sets: [{
                title: qsTr("default"),
                pages: [{
                    name: oldJson.page || qsTr("page 1"),
                    pin: false,
                    speed: 1.0,
                    items: items
                }]
            }]
        };
    }

    function buildDefaultSet() {
        return {
            title: qsTr("default"),
            pages: [
                { name: "R", pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.att.roll.value", title: qsTr("roll cmd") },
                    { bind: "mandala.est.att.roll.value", title: qsTr("roll") }
                ]},
                { name: "P", pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.att.pitch.value", title: qsTr("pitch cmd") },
                    { bind: "mandala.est.att.pitch.value", title: qsTr("pitch") }
                ]},
                { name: "Y", pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.pos.bearing.value", title: qsTr("bearing cmd") },
                    { bind: "mandala.cmd.att.yaw.value", title: qsTr("yaw cmd") },
                    { bind: "mandala.est.att.yaw.value", title: qsTr("yaw") }
                ]},
                { name: "Axy", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.acc.x.value", title: qsTr("Ax") },
                    { bind: "mandala.est.acc.y.value", title: qsTr("Ay") }
                ]},
                { name: "Az", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.acc.z.value", title: qsTr("Az") }
                ]},
                { name: "G", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.gyro.x.value", title: qsTr("Gx") },
                    { bind: "mandala.est.gyro.y.value", title: qsTr("Gy") },
                    { bind: "mandala.est.gyro.z.value", title: qsTr("Gz") }
                ]},
                { name: "Pt", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.pos.altitude.value", title: qsTr("alt") },
                    { bind: "mandala.est.pos.vspeed.value", title: qsTr("vspd") },
                    { bind: "mandala.est.air.airspeed.value", title: qsTr("airspeed") }
                ]},
                { name: "Ctr", pin: false, speed: 1.0, items: [
                    { bind: "mandala.ctr.att.ail.value", title: qsTr("ail") },
                    { bind: "mandala.ctr.att.elv.value", title: qsTr("elv") },
                    { bind: "mandala.ctr.att.rud.value", title: qsTr("rud") },
                    { bind: "mandala.ctr.eng.thr.value", title: qsTr("thr") },
                    { bind: "mandala.ctr.eng.prop.value", title: qsTr("prop") },
                    { bind: "mandala.ctr.str.rud.value", title: qsTr("str.rud") }
                ]},
                { name: "RC", pin: false, speed: 1.0, items: [
                    { bind: "mandala.cmd.rc.roll.value", title: qsTr("RC roll") },
                    { bind: "mandala.cmd.rc.pitch.value", title: qsTr("RC pitch") },
                    { bind: "mandala.cmd.rc.thr.value", title: qsTr("RC thr") },
                    { bind: "mandala.cmd.rc.yaw.value", title: qsTr("RC yaw") }
                ]},
                { name: "Usr", pin: false, speed: 1.0, items: [
                    { bind: "mandala.est.usr.u1.value", title: qsTr("u1") },
                    { bind: "mandala.est.usr.u2.value", title: qsTr("u2") },
                    { bind: "mandala.est.usr.u3.value", title: qsTr("u3") },
                    { bind: "mandala.est.usr.u4.value", title: qsTr("u4") },
                    { bind: "mandala.est.usr.u5.value", title: qsTr("u5") },
                    { bind: "mandala.est.usr.u6.value", title: qsTr("u6") }
                ]}
            ]
        };
    }
}
