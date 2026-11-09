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
import Qt.labs.folderlistmodel

import Apx.Application
import APX.Facts

import "ui"
import "ChecklistParser.js" as Parser

/*
    Checklist plugin: aircraft check procedure driven by a Markdown file.

    Procedures live in <Documents>/UAVOS/Checklists/*.md. The first file (by name) whose name is
    the beginning of the current unit callsign (case insensitive: Borey-123 -> borey.md) is the
    procedure of that unit, checklist.md is the default when nothing matches; while a procedure
    exists a launcher button is shown in the task bar and opens the window.
    This file is the engine: it parses the Markdown (ChecklistParser.js), fills {{expression}}
    placeholders of the texts and runs the steps one by one. The window (ui/ChecklistWindow.qml)
    only displays `doc`, `results`, `buttons`, `form`, `message` and calls back
    start/abort/answer/submit/retry/rerun/proceed.

    All scripts run in the global scope of the GCS JS engine, the same one as the Terminal
    console: the shared block is executed once per run (and once on load, for the {{}} texts),
    the variables and functions of a step stay visible to the following steps. The step script
    gets the global `step` object (see `api` below) and finishes the step with step.pass()/
    step.fail(); when it returns with nothing pending (no buttons, no form, no poll) the step
    passes. A step without a script shows Confirm.
*/

AppPlugin {
    id: plugin

    title: qsTr("Checklist")
    descr: qsTr("Aircraft check procedures")
    icon: "clipboard-check"

    // ---------------------------------------------------------------- procedure file
    readonly property string dir: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/UAVOS/Checklists"
    readonly property string callsign: apx.fleet.current ? apx.fleet.current.title : ""
    property string file: ""         // procedure of the current unit, empty when there is none

    FolderListModel {
        id: folder
        folder: plugin.dir
        nameFilters: ["*.md"]
        caseSensitive: false
        sortField: FolderListModel.Name
        sortCaseSensitive: false
        showDirs: false
        onCountChanged: plugin.match()
        onStatusChanged: if(status === FolderListModel.Ready) plugin.match()
    }
    onCallsignChanged: match()
    // the first file (by name) that starts the callsign, checklist.md when none does
    function match()
    {
        var cs = callsign.toLowerCase()
        var found = "", fallback = ""
        for (var i = 0; i < folder.count; ++i) {
            var n = String(folder.get(i, "fileBaseName")).toLowerCase()
            var path = String(folder.get(i, "filePath"))
            if(n === "checklist"){
                if(!fallback) fallback = path
                continue
            }
            if(!found && n && cs && cs.indexOf(n) === 0) found = path
        }
        file = found ? found : fallback
    }
    onFileChanged: {
        if(!file) shown = false
        else if(shown) load()
    }

    // ---------------------------------------------------------------- task bar button and window
    Fact {
        id: f_launcher
        property bool showLauncher: true   // task bar button (see TaskBar.qml)
        name: "checklist"
        title: qsTr("Checklist")
        descr: callsign + ": " + file
        icon: "clipboard-check"
        parentFact: plugin.file ? apx.windows : null   // a child (not an Action): the task bar lists apx.windows children
        onTriggered: plugin.open()
    }
    property bool shown: false
    function open()
    {
        if(!file) return
        if(!shown){
            load()
            shown = true
        }
        if(window.item){
            window.item.raise()
            window.item.requestActivate()
        }
    }
    Loader {
        id: window
        active: plugin.shown
        sourceComponent: ChecklistWindow {
            checklist: plugin
            onClosing: Qt.callLater(function(){ plugin.shown = false })
        }
    }
    onShownChanged: if(!shown) reset()

    // reachable from QML and scripts as ui.checklist (phase, takeoffAllowed)
    Component.onCompleted: ui.checklist = plugin

    // ---------------------------------------------------------------- procedure
    property var doc: ({ "title": "", "intro": "", "shared": "", "steps": [] })
    readonly property var steps: doc.steps     // [{title, body, code}]

    function load()
    {
        reset()
        var text = file ? application.loadText(file) : ""
        if(file && !text) console.warn("Checklist: can't read", file)
        var d = Parser.parse(text)
        // {{expression}} in titles and descriptions is replaced by its value, shared block variables are visible
        var err = runShared(d.shared)
        if(err){
            console.warn("Checklist: shared block error:", err)
        }else{
            d.title = expand(d.title)
            d.intro = expand(d.intro)
            d.steps.forEach(function(st){
                st.title = expand(st.title)
                st.body = expand(st.body)
            })
        }
        doc = d
    }
    function expand(text)
    {
        return text.replace(/\{\{([\s\S]+?)\}\}/g, function(m, e){
            var v = application.jsexec("(" + e + "\n)")
            if(errorOf(v)) return "{{" + errorOf(v) + "}}"
            return v === undefined || v === null ? "" : String(v)
        })
    }

    // ---------------------------------------------------------------- scripts environment
    // scripts are evaluated in the global JS scope (as Terminal commands); `step` is the api object
    function runShared(code)
    {
        ui.checklist = plugin   // may run before Component.onCompleted (persistent setting restored)
        application.jsexec("var step = ui.checklist.api")
        return errorOf(application.jsexec(code))
    }
    // error text of a jsexec result, "" when the script completed
    function errorOf(v)
    {
        if(v instanceof Error) return String(v)
        if(v && typeof(v) === "object" && typeof(v.name) === "string" && /Error$/.test(v.name) && v.message !== undefined) return v.name + ": " + v.message
        return ""
    }

    // ---------------------------------------------------------------- run phase
    property string phase: "idle"    // idle | running | finished
    property int current: -1
    property var results: []         // by step index: {status: ok|fail, message}
    property string message: ""      // live text of the running step
    property var buttons: []         // [{text, kind}] requested by the step script
    property var form: null          // {label: default} requested by the step script
    property bool takeoffAllowed: false

    readonly property var step: (current >= 0 && current < steps.length) ? steps[current] : null
    readonly property bool failed: !!(step && results[current] && results[current].status === "fail")
    readonly property bool pending: buttons.length > 0 || form !== null || pollTimer.running

    // script callbacks of the running step
    property var _askHandler: null
    property var _formHandler: null
    property var _pollCond: null
    property var _pollThen: null
    property var _cleanup: null
    property bool _done: false       // the running step has passed or failed
    property int _returnTo: -1       // step to continue with after a repeated earlier step

    // ---------------------------------------------------------------- api for step scripts
    readonly property var api: ({
        // buttons for the operator, handler(answer) is called on press;
        // items are strings or {text, kind, restart}, kind: ok (default), primary, danger, warn, neutral;
        // a button with restart: true restarts the step instead of calling the handler
        "ask": function(list, handler) {
            plugin._askHandler = handler ? handler : null
            plugin.buttons = list.map(function(b) {
                if(b && typeof(b) === "object") return { "text": String(b.text), "kind": b.kind ? String(b.kind) : "ok", "restart": !!b.restart }
                return { "text": String(b), "kind": "ok", "restart": false }
            })
            plugin.form = null
        },
        // input fields {label: default}, handler(values) is called on submit
        "form": function(fields, handler) {
            plugin._formHandler = handler ? handler : null
            plugin.form = fields
            plugin.buttons = []
        },
        // live text under the step description
        "info": function(msg) { plugin.message = msg ? String(msg) : "" },
        // cond() is evaluated every 0.5 s up to sec seconds (0: forever);
        // true -> then() or pass, timeout -> fail
        "poll": function(cond, sec, then) {
            plugin._pollCond = cond
            plugin._pollThen = then ? then : null
            plugin.pollTimer.deadline = sec > 0 ? Date.now() + sec*1000 : 0
            plugin.pollTimer.restart()
        },
        // fn() is called when the step ends for any reason (pass, fail, restart, abort)
        "cleanup": function(fn) { plugin._cleanup = fn },
        // run the step from the beginning
        "restart": function() { plugin.retry() },
        "pass": function(msg) { plugin.pass(msg) },
        "fail": function(msg) { plugin.fail(msg) },
    })

    // ---------------------------------------------------------------- procedure control
    function start()
    {
        reset()
        _sharedError = runShared(doc.shared)
        phase = "running"
        next()
    }
    property string _sharedError: ""   // the shared block failed on start: every step fails with it
    function abort()
    {
        if(phase !== "running") return
        endStep()
        setResult(current, "fail", qsTr("Aborted by operator"))
        finish()
    }
    function finish()
    {
        endStep()
        _returnTo = -1
        phase = "finished"
        takeoffAllowed = steps.length > 0 && !results.some(function(r){ return r && r.status === "fail" })
    }
    function reset()
    {
        endStep()
        phase = "idle"
        takeoffAllowed = false
        current = -1
        _returnTo = -1
        results = []
        message = ""
        _done = false
    }

    // ---------------------------------------------------------------- step control
    function next()
    {
        endStep()
        if(_returnTo >= 0){
            // a repeated earlier step is done: back to the step that was interrupted
            goTo(_returnTo)
            _returnTo = -1
            return
        }
        if(current + 1 >= steps.length){
            finish()
            return
        }
        goTo(current + 1)
    }
    // restart the current step (also used as Retry after a failure)
    function retry()
    {
        if(phase !== "running" || !step) return
        goTo(current)
    }
    // repeat an already passed step, then continue with the current one
    function rerun(index)
    {
        if(phase !== "running" || index < 0 || index >= current) return
        if(_returnTo < 0) _returnTo = current
        goTo(index)
    }
    // go on after a failed step
    function proceed()
    {
        if(phase !== "running" || !failed) return
        next()
    }
    function goTo(index)
    {
        endStep()
        current = index
        setResult(current, "", "")
        runStep()
    }
    function runStep()
    {
        message = ""
        _done = false
        if(!step.code.trim()){
            api.ask([qsTr("Confirm")], null)
            return
        }
        if(_sharedError){
            fail(qsTr("Shared block error") + ": " + _sharedError)
            return
        }
        var err = errorOf(application.jsexec(step.code))
        if(err){
            fail(qsTr("Script error") + ": " + err)
            return
        }
        settle()
    }
    // run a script callback: errors fail the step
    function call(fn, arg)
    {
        if(!fn) return
        try {
            fn(arg)
        } catch(e) {
            fail(qsTr("Script error") + ": " + String(e))
            return
        }
        settle()
    }
    // a script that returned with nothing pending passes the step
    function settle()
    {
        if(phase === "running" && !_done && !pending) pass(message)
    }
    // operator pressed a button (by its text)
    function answer(button)
    {
        if(phase !== "running" || _done) return
        if(buttons.some(function(b){ return b.text === button && b.restart })){
            retry()
            return
        }
        var h = _askHandler
        _askHandler = null
        buttons = []
        call(h, button)
    }
    // operator submitted the form
    function submit(values)
    {
        if(phase !== "running" || _done) return
        var h = _formHandler
        _formHandler = null
        form = null
        call(h, values)
    }
    function pass(msg)
    {
        if(_done) return
        _done = true
        endStep()
        setResult(current, "ok", msg)
        next()
    }
    function fail(msg)
    {
        if(_done) return
        _done = true
        endStep()
        setResult(current, "fail", msg ? msg : (message ? message : qsTr("Check failed")))
    }
    // stop everything the step script has requested
    function endStep()
    {
        pollTimer.stop()
        _pollCond = null
        _pollThen = null
        _askHandler = null
        _formHandler = null
        buttons = []
        form = null
        var c = _cleanup
        _cleanup = null
        if(c){
            try { c() } catch(e) { console.warn("Checklist: cleanup error:", e) }
        }
    }
    // status "" clears the result of the step
    function setResult(index, status, msg)
    {
        if(index < 0) return
        var r = results.slice()
        r[index] = status ? { "status": status, "message": msg ? String(msg) : "" } : undefined
        results = r
    }

    property Timer pollTimer: Timer {
        interval: 500
        repeat: true
        property real deadline: 0
        onTriggered: {
            if(plugin.phase !== "running" || plugin._done || !plugin._pollCond){
                stop()
                return
            }
            var ok = false
            try {
                ok = !!plugin._pollCond()
            } catch(e) {
                stop()
                plugin.fail(qsTr("Script error") + ": " + String(e))
                return
            }
            if(ok){
                stop()
                var then = plugin._pollThen
                plugin._pollCond = null
                plugin._pollThen = null
                if(then) plugin.call(then, undefined)
                else plugin.pass(plugin.message)
                return
            }
            if(deadline > 0 && Date.now() >= deadline){
                stop()
                plugin.fail(plugin.message ? plugin.message : qsTr("Timeout"))
            }
        }
    }
}
