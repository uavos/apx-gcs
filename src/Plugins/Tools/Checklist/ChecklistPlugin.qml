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

import Apx.Application
import APX.Facts

import "ui"
import "ChecklistParser.js" as Parser

/*
    Checklist plugin: aircraft check procedure driven by Checklist.md (plugin resources).

    Tools -> Checklist -> Enabled (off by default) shows the window and loads the procedure.
    This file is the engine: it parses the Markdown (ChecklistParser.js), fills {{expression}}
    placeholders of the texts from the shared block scope and runs the steps one by one. The window (ui/ChecklistWindow.qml) only displays `doc`, `results`, `buttons`,
    `form`, `message` and calls back start/abort/answer/submit/retry/rerun/proceed.

    A step script runs in the GCS JS engine with the `step` object (see `api` below).
    It finishes the step with step.pass()/step.fail(); when it returns with nothing pending
    (no buttons, no form, no poll) the step passes. A step without a script shows Confirm.
*/

AppPlugin {
    id: plugin

    title: qsTr("Checklist")
    descr: qsTr("Aircraft check procedures")
    icon: "clipboard-check"

    property string file: "qrc:/Checklist/Checklist.md"

    // ---------------------------------------------------------------- tools menu
    Fact {
        name: "checklist"
        title: qsTr("Checklist")
        descr: qsTr("Aircraft check procedure")
        icon: "clipboard-check"
        flags: Fact.Group
        section: qsTr("Tools")
        Component.onCompleted: parentFact = apx.tools
        Fact {
            id: f_enabled
            name: "use"
            title: qsTr("Enabled")
            descr: qsTr("Show the checklist window")
            flags: Fact.Bool | Fact.PersistentValue
            defaultValue: false
        }
    }
    readonly property bool opened: f_enabled.value ? true : false
    onOpenedChanged: opened ? load() : reset()

    // the window lives while the tool is enabled; closing it disables the tool
    Loader {
        active: plugin.opened
        sourceComponent: ChecklistWindow {
            checklist: plugin
            onClosing: Qt.callLater(function(){ f_enabled.value = false })
        }
    }

    // reachable from QML and scripts as ui.checklist (phase, takeoffAllowed)
    Component.onCompleted: {
        ui.checklist = plugin
        if(opened) load()
    }

    // ---------------------------------------------------------------- procedure
    property var doc: ({ "title": "", "intro": "", "shared": "", "steps": [] })
    readonly property var steps: doc.steps     // [{title, body, code}]

    function load()
    {
        reset()
        var text = application.loadText(file)
        if(!text) console.warn("Checklist: can't read", file)
        var d = Parser.parse(text)
        // {{expression}} in titles and descriptions is replaced by its value from the shared block scope
        var ev = application.jsexec("(function(){\n" + d.shared + "\nreturn function(__e){ return eval(__e) }\n})()")
        if(typeof(ev) !== "function"){
            console.warn("Checklist: shared block error:", String(ev))
            ev = null
        }
        d.title = expand(d.title, ev)
        d.intro = expand(d.intro, ev)
        d.steps.forEach(function(st){
            st.title = expand(st.title, ev)
            st.body = expand(st.body, ev)
        })
        doc = d
    }
    function expand(text, ev)
    {
        return text.replace(/\{\{([\s\S]+?)\}\}/g, function(m, e){
            if(!ev) return m
            try {
                var v = ev(e)
                return v === undefined || v === null ? "" : String(v)
            } catch(err) {
                return "{{" + String(err) + "}}"
            }
        })
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
        phase = "running"
        next()
    }
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
        var fn = application.jsexec("(function(step){\n" + doc.shared + "\n" + step.code + "\n})")
        if(typeof(fn) !== "function"){
            fail(qsTr("Script error") + ": " + String(fn))
            return
        }
        call(fn, api)
    }
    // run a script callback: errors fail the step, nothing pending passes it
    function call(fn, arg)
    {
        if(!fn) return
        try {
            fn(arg)
        } catch(e) {
            fail(qsTr("Script error") + ": " + String(e))
            return
        }
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
