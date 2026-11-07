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
    Checklist engine: parses the Markdown procedure and runs its steps one by one.

    Markdown format (nothing else is parsed):
      # Title                 procedure title, text below it is the description
      ```js ... ```           in the description: shared code, prepended to every step script
      ## Step title           a step, text below it is shown to the operator
      ```js ... ```           in a step: the step script, gets the `step` object (see api below)

    A step without a script shows a single Confirm button.
    A script finishes the step with step.pass()/step.fail(); when it returns with nothing
    pending (no buttons, no form, no poll) the step passes.
*/

QtObject {
    id: runner

    property string source: "qrc:/Checklist/Checklist.md"

    // procedure
    property string title: ""
    property string intro: ""        // markdown between the title and the first step
    property string shared: ""       // js from the description
    property var steps: []           // {title, body, code}

    // run state
    property string state: "idle"    // idle | running | finished
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
        // items are strings or {text, kind}, kind: ok (default), primary, danger, neutral
        "ask": function(list, handler) {
            runner._askHandler = handler ? handler : null
            runner.buttons = list.map(function(b) {
                if(b && typeof(b) === "object") return { "text": String(b.text), "kind": b.kind ? String(b.kind) : "ok" }
                return { "text": String(b), "kind": "ok" }
            })
            runner.form = null
        },
        // input fields {label: default}, handler(values) is called on submit
        "form": function(fields, handler) {
            runner._formHandler = handler ? handler : null
            runner.form = fields
            runner.buttons = []
        },
        // live text under the step description
        "info": function(msg) { runner.message = msg ? String(msg) : "" },
        // cond() is evaluated every 0.5 s up to sec seconds (0: forever);
        // true -> then() or pass, timeout -> fail
        "poll": function(cond, sec, then) {
            runner._pollCond = cond
            runner._pollThen = then ? then : null
            runner.pollTimer.deadline = sec > 0 ? Date.now() + sec*1000 : 0
            runner.pollTimer.restart()
        },
        // fn() is called when the step ends for any reason (pass, fail, restart, abort)
        "cleanup": function(fn) { runner._cleanup = fn },
        "pass": function(msg) { runner.pass(msg) },
        "fail": function(msg) { runner.fail(msg) },
    })

    // ---------------------------------------------------------------- markdown
    function load()
    {
        var text = application.loadText(source)
        if(!text) console.warn("Checklist: can't read", source)
        var doc = parse(text)
        title = doc.title
        intro = doc.intro
        shared = doc.shared
        steps = doc.steps
    }
    function parse(text)
    {
        var doc = { "title": "", "intro": "", "shared": "", "steps": [] }
        var lines = text.split(/\r?\n/)
        var step = null      // current step, null while in the description
        var body = []        // markdown lines of the current section
        var code = null      // lines of the open ```js block
        function flush()
        {
            var md = body.join("\n").trim()
            if(step) step.body = md
            else doc.intro = md
            body = []
        }
        for (var i = 0; i < lines.length; ++i) {
            var line = lines[i]
            if(code !== null){
                if(/^```/.test(line)){
                    var js = code.join("\n") + "\n"
                    if(step) step.code += js
                    else doc.shared += js
                    code = null
                }else code.push(line)
                continue
            }
            if(/^```\s*js\b/.test(line)){
                code = []
                continue
            }
            if(/^#\s+/.test(line)){
                doc.title = line.replace(/^#\s+/, "").trim()
                continue
            }
            if(/^##\s+/.test(line)){
                flush()
                step = { "title": line.replace(/^##\s+/, "").trim(), "body": "", "code": "" }
                doc.steps.push(step)
                continue
            }
            body.push(line)
        }
        flush()
        return doc
    }

    // ---------------------------------------------------------------- procedure control
    function start()
    {
        reset()
        load()
        state = "running"
        results = []
        next()
    }
    function abort()
    {
        if(state !== "running") return
        endStep()
        setResult(current, "fail", qsTr("Aborted by operator"))
        finish()
    }
    function finish()
    {
        endStep()
        _returnTo = -1
        state = "finished"
        takeoffAllowed = steps.length > 0 && !results.some(function(r){ return r && r.status === "fail" })
    }
    function reset()
    {
        endStep()
        state = "idle"
        takeoffAllowed = false
        current = -1
        _returnTo = -1
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
        if(state !== "running" || !step) return
        goTo(current)
    }
    // repeat an already passed step, then continue with the current one
    function rerun(index)
    {
        if(state !== "running" || index < 0 || index >= current) return
        if(_returnTo < 0) _returnTo = current
        goTo(index)
    }
    // go on after a failed step
    function proceed()
    {
        if(state !== "running" || !failed) return
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
        var fn = application.jsexec("(function(step){\n" + shared + "\n" + step.code + "\n})")
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
        if(state === "running" && !_done && !pending) pass(message)
    }
    // operator pressed a button
    function answer(button)
    {
        if(state !== "running" || _done) return
        var h = _askHandler
        _askHandler = null
        buttons = []
        call(h, button)
    }
    // operator submitted the form
    function submit(values)
    {
        if(state !== "running" || _done) return
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
            if(runner.state !== "running" || runner._done || !runner._pollCond){
                stop()
                return
            }
            var ok = false
            try {
                ok = !!runner._pollCond()
            } catch(e) {
                stop()
                runner.fail(qsTr("Script error") + ": " + String(e))
                return
            }
            if(ok){
                stop()
                var then = runner._pollThen
                runner._pollCond = null
                runner._pollThen = null
                if(then) runner.call(then, undefined)
                else runner.pass(runner.message)
                return
            }
            if(deadline > 0 && Date.now() >= deadline){
                stop()
                runner.fail(runner.message ? runner.message : qsTr("Timeout"))
            }
        }
    }
}
