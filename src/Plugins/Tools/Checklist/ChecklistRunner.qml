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

/*
    Checklist engine: parses the Markdown procedure and runs its steps.

    Step options (key: value lines right after the "## " heading):
      type: auto | confirm      auto - the "check" snippet decides, confirm - operator presses a button
      fail: stop | continue     what happens when the step fails (default continue)
      buttons: A, B, C                      buttons for confirm steps (default: Confirm)
      wait: 30                              auto step: poll "check" up to N seconds
    Markdown between "# title" and the first step is the procedure description.
    Headings "###" and deeper are ordinary text inside descriptions.
    Snippets: ```js check```, ```js start```, ```js confirm```, ```js cleanup```
    run in the GCS JS engine with globals cfg (checklist.json) and answer (pressed button).
*/

QtObject {
    id: runner

    property string dir
    readonly property string mdFile: dir + "/checklist.md"
    readonly property string cfgFile: dir + "/checklist.json"

    property string title: ""
    property string intro: ""        // markdown between the title and the first step
    property var steps: []
    property var config: ({})

    // run state
    property string state: "idle"          // idle | running | finished
    property int current: -1
    property string message: ""
    property bool takeoffAllowed: false
    property var results: []               // {title, status, message}
    readonly property var step: (current >= 0 && current < steps.length) ? steps[current] : null

    signal changed()

    // ---------------------------------------------------------------- files
    function loadFiles()
    {
        var text = application.loadText(mdFile)
        if(!text){
            text = application.loadText("qrc:/Checklist/Checklist.md")
            application.saveText(mdFile, text)
        }
        var doc = parse(text)
        title = doc.title
        intro = doc.intro
        steps = doc.steps

        var obj = null
        var json = application.loadText(cfgFile)
        if(json){
            try { obj = JSON.parse(json) } catch(e) { console.warn("Checklist: config parse error:", e) }
        }else{
            obj = defaultConfig()
            application.saveText(cfgFile, JSON.stringify(obj, null, 2))
        }
        config = obj ? obj : defaultConfig()
        application.jsexec("cfg=" + JSON.stringify(config))
    }
    function defaultConfig()
    {
        return {
            "batteryMin": 15.0,
            "fuelMin": 10,
            "ekfMagMax": 1,
            "gpsMinSats": 6,
            "engineStatusPath": "est.usrc.c2",
            "magToleranceDeg": 15,
            "airspeedZeroMax": 3,
            "airspeedBlowMin": 5,
            "tiltMinDeg": 10,
            "tiltMaxDeg": 30,
            "levelMaxDeg": 5,
            "catapultHeadingToleranceDeg": 10,
            "diveAltitudeMin": 1000,
            "ranges": [
                { "path": "sns.pwr.vsys", "min": 14, "max": 17.5 },
                { "path": "sns.pwr.vsrv", "min": 7, "max": 9 }
            ]
        }
    }

    // ---------------------------------------------------------------- markdown
    function parse(text)
    {
        var doc = { "title": "", "intro": "", "steps": [] }
        var lines = text.split(/\r?\n/)
        var step = null
        var body = []
        var code = null
        var role = ""
        function flush()
        {
            if(!step) return
            step.body = body.join("\n").trim()
            body = []
        }
        for (var i = 0; i < lines.length; ++i) {
            var line = lines[i]
            if(code !== null){
                if(/^```/.test(line)){
                    if(step) step[role] = code.join("\n")
                    code = null
                }else code.push(line)
                continue
            }
            var m = line.match(/^```\s*js\s+(check|start|confirm|cleanup)\s*$/)
            if(m && step){
                code = []
                role = m[1]
                continue
            }
            if(/^#\s+/.test(line)){
                doc.title = line.replace(/^#\s+/, "").trim()
                continue
            }
            if(/^##\s+/.test(line)){
                if(step) flush()
                else { doc.intro = body.join("\n").trim(); body = [] }
                step = { "title": line.replace(/^##\s+/, "").trim(), "type": "confirm", "fail": "continue",
                         "buttons": [], "wait": 0, "body": "",
                         "check": "", "start": "", "confirm": "", "cleanup": "" }
                doc.steps.push(step)
                continue
            }
            m = line.match(/^(type|fail|buttons|wait):\s*(.*)$/)
            if(m && step && body.length === 0){
                var v = m[2].trim()
                if(m[1] === "buttons") step.buttons = v.split(",").map(function(s){ return s.trim() }).filter(function(s){ return s })
                else if(m[1] === "wait") step.wait = Number(v)
                else step[m[1]] = v
                continue
            }
            if(step) body.push(line)
            else body.push(line)   // before the first step: procedure description
        }
        if(step) flush()
        else doc.intro = body.join("\n").trim()
        return doc
    }

    // ---------------------------------------------------------------- js
    function exec(code, extra)
    {
        if(!code) return true
        if(extra) application.jsexec(extra)
        var r = application.jsexec(code)
        if(r && typeof(r) === "object" && r.ok !== undefined){
            if(r.msg) message = String(r.msg)
            return !!r.ok
        }
        if(r instanceof Error){
            message = String(r)
            return false
        }
        return !!r
    }

    // ---------------------------------------------------------------- run
    function start()
    {
        stopTimers()
        loadFiles()
        state = "running"
        takeoffAllowed = false
        results = []
        message = ""
        current = -1
        next()
    }
    function abort()
    {
        if(state !== "running") return
        cleanupStep()
        setResult("fail", qsTr("Aborted by operator"))
        finish()
    }
    function next()
    {
        stopTimers()
        current++
        if(current >= steps.length){
            finish()
            return
        }
        message = ""
        exec(step.start)
        if(step.type === "auto")
            runCheck()
        changed()
    }
    function runCheck()
    {
        if(exec(step.check)){
            pass()
            return
        }
        if(step.wait > 0 && pollTimer.elapsed < step.wait*1000){
            pollTimer.start()
            return
        }
        fail(message ? message : qsTr("Check failed"))
    }
    function answer(button)
    {
        if(state !== "running" || !step) return
        if(exec(step.confirm, "answer=" + JSON.stringify(button)))
            pass()
        else
            fail(message ? message : qsTr("Not confirmed") + ": " + button)
    }
    function retry()
    {
        if(state !== "running" || !step) return
        pollTimer.elapsed = 0
        message = ""
        if(step.type === "auto") runCheck()
        changed()
    }
    function pass()
    {
        cleanupStep()
        setResult("ok", message)
        next()
    }
    function fail(msg)
    {
        cleanupStep()
        setResult("fail", msg)
        if(step.fail === "stop"){
            finish()
            return
        }
        next()
    }
    function cleanupStep()
    {
        stopTimers()
        if(step) exec(step.cleanup)
    }
    function setResult(status, msg)
    {
        var r = results.slice()
        r.push({ "title": step ? step.title : "", "status": status, "message": msg ? msg : "" })
        results = r
    }
    function finish()
    {
        stopTimers()
        state = "finished"
        takeoffAllowed = !results.some(function(r){ return r.status === "fail" })
        saveReport()
        changed()
    }
    function saveReport()
    {
        var d = new Date()
        var stamp = d.toISOString().replace(/[:.]/g, "-").slice(0, 19)
        var report = { "time": d.toISOString(), "passed": takeoffAllowed, "steps": results }
        application.saveText(dir + "/reports/" + stamp + ".json", JSON.stringify(report, null, 2))
    }
    function stopTimers()
    {
        pollTimer.stop()
        pollTimer.elapsed = 0
    }

    property Timer pollTimer: Timer {
        interval: 500
        repeat: false
        property int elapsed: 0
        onTriggered: {
            elapsed += interval
            runner.runCheck()
        }
    }
}
