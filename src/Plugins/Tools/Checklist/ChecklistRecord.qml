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
import QtCore

/*
    Evidence of one check: the folder <Documents>/UAVOS/Checklists/<basename>/ where
    <basename> is the telemetry file name style (yyMMdd_HHmm_sszzz + UTC offset + callsign).
    log(text) appends a timestamped line to <basename>.log, snapshot() saves
    <basename>-NNN.jpg from the camera when the operator has agreed to it.
*/
QtObject {
    id: record

    property ChecklistCamera camera: ChecklistCamera {
        onSaved: (path) => record.log("snapshot saved: " + path)
        onFailed: (message) => record.log("snapshot failed: " + message)
    }

    readonly property string root: String(StandardPaths.writableLocation(StandardPaths.DocumentsLocation)).replace(/^file:\/\//, "") + "/UAVOS/Checklists"
    property string base: ""        // run name
    property string dir: ""         // run folder, empty until start()
    property int shots: 0

    function start(callsign)
    {
        base = basename(new Date(), callsign)
        dir = root + "/" + base
        shots = 0
    }
    function basename(d, callsign)
    {
        function p(v, n) { var s = String(v); while(s.length < n) s = "0" + s; return s }
        var ts = p(d.getFullYear() % 100, 2) + p(d.getMonth() + 1, 2) + p(d.getDate(), 2)
               + "_" + p(d.getHours(), 2) + p(d.getMinutes(), 2)
               + "_" + p(d.getSeconds(), 2) + p(d.getMilliseconds(), 3)
        var off = -d.getTimezoneOffset()   // minutes east of UTC
        var mins = Math.abs(off)
        ts += (off > 0 ? "E" : "W") + (mins % 60 ? mins : mins / 60)
        var cs = String(callsign).toUpperCase().replace(/[^A-Z0-9]/g, "")
        return ts + "-" + (cs ? cs : "U")
    }
    function timestamp()
    {
        var d = new Date()
        function p(v, n) { var s = String(v); while(s.length < n) s = "0" + s; return s }
        return p(d.getHours(), 2) + ":" + p(d.getMinutes(), 2) + ":" + p(d.getSeconds(), 2) + "." + p(d.getMilliseconds(), 3)
    }
    // a line of the text log of the run
    function log(text)
    {
        if(!dir) return
        if(!application.appendText(dir + "/" + base + ".log", timestamp() + " " + text))
            console.warn("Checklist: can't write", dir)
    }
    // a camera snapshot, "" when the camera is off
    function snapshot(tag)
    {
        if(!dir || !camera.enabled) return ""
        shots += 1
        var n = String(shots)
        while(n.length < 3) n = "0" + n
        var path = dir + "/" + base + "-" + n + ".jpg"
        log("snapshot " + n + " (" + tag + "): " + path)
        if(!camera.capture(path)) return ""
        return path
    }
}
