---
page: plugins
---

# Checklist

Aircraft check procedure driven by a Markdown file with embedded JS snippets.

Procedures are the `*.md` files in `<Documents>/UAVOS/Checklists/`. The file is picked by
the callsign of the current unit: its name must be the beginning of the callsign, case
insensitive (`Borey-123` opens `borey.md`; when several names match the first one by name
is used). While a procedure exists and the check can be started, a Checklist button is
shown in the task bar; it opens the window with the procedure. No file, no button.

The check is started on the ground only: the button and **Start** are available while the
unit is in TAXI, not in the air (`cmd.ins.inair`, `est.sys.mode`) and the uplink is enabled;
otherwise the window names the reason. A running check is never interrupted by the engine:
mode changes and the takeoff are left to the procedure (its steps switch EMG, RPV, TAKEOFF
themselves, and may end the check with the launch). A procedure that must keep the aircraft
on the ground while it runs blocks the takeoff itself (see the guard example below). The
main layout flag `checklistAuto` (set by the lite
profile, `resources/layouts/lite/GroundControl.qml`) opens the checklist by itself when
the unit enters TAXI; the stock layout only shows the button.

Every check is recorded. The window opens with the start page: the operator enters their
full name (mandatory, nothing starts without it) and answers the camera question - picks
one of the video inputs and agrees, or declines; the name and the answer are the first
lines of the record.
The record is the folder `<Documents>/UAVOS/Checklists/<run>/`, `<run>` in the telemetry
file name style plus the operator (`261030_1304_08119W4-BOREY-Ivanov_I_I`): `<run>.log`
gets a timestamped line for
everything that happens (start, each step started/passed/failed with its message, answers,
form values, info messages, `step.log` notes, snapshots, the result), and `<run>-NNN.jpg` is
the camera snapshot taken after every step when the camera is on. Step starts, results and
`step.log` notes also go to the telemetry file as msg events (src `gcs/checklist`).

Steps run strictly in order: the current step is highlighted, only the steps already
reached can be opened, none can be skipped. A failed step is retried with **Retry** or
passed over with **Continue**, a passed step is repeated with **Repeat step** (the check
returns to the current step afterwards). A running step offers no restart button of its
own: the script adds one when it makes sense (`restart: true` in `step.ask`). Closing the
window aborts the check.

Files, loaded in stages so that nothing exists before it is needed:

- `ChecklistPlugin.qml` - stage 1, always present: finds the procedure file of the current
  unit and loads the launcher while one exists;
- `ChecklistLauncher.qml` - stage 2, the task bar button; loads the checklist on press;
- `Checklist.qml` - stage 3, the engine and its window, destroyed when the window is closed.
  Reachable from QML and scripts as `ui.checklist` (`phase`, `result`) while open; `result`
  is `""` until the check has finished, then `"passed"` (every step ok) or `"failed"`;
- `ChecklistRecord.qml` - the record of a run: folder, text log, snapshots;
  `ChecklistCamera.qml` - the snapshot camera (QtMultimedia), `ui/ChecklistCameraPage.qml`
  the consent page;
- `ChecklistParser.js` - the Markdown parser, `parse(text)` returns
  `{title, intro, steps: [{title, body, code}]}`;
- `ui/` - the window and its widgets. The window shows what the engine exposes
  (`doc`, `results`, `buttons`, `form`, `message`) and knows nothing about Markdown;
  colors and sizes are in `ui/ChecklistTheme.qml`, button kinds in `ui/ChecklistButton.qml`.

Markdown format, nothing else is parsed:

- `# Title` - procedure title, the text below it is the description;
- `## Step` - a step, the text below it is shown to the operator;
- `{{expression}}` is replaced by the value of the JS expression, evaluated in the global
  scope. Everything is expanded when the procedure is loaded, and an expression that cannot
  be evaluated yet is kept as typed; the step body is expanded again when the step becomes
  current, so it sees whatever the scripts of the earlier steps have declared - any variable
  or function, not just a config object (an error at that point is shown in place). Titles
  are not re-expanded, dynamic values belong in the body or in `step.info`;
- a ```` ```js ```` block in a step is the step script. All scripts run in the global scope
  of the GCS JS engine, the same as the Terminal console (`cmd.proc.mode`, `sns.pwr.vsys`,
  `apx`...): what a step declares stays visible to the following steps, so thresholds and
  helpers used by several steps are declared in the first step that needs them, and a value
  chosen in one step (`mode = answer`) can be checked in a later one (`if(mode === ...)`). Names of
  GCS globals are write-protected; a top-level `return` is not allowed. The script uses the
  global `step` object:
  - `step.ask(["A", {text: "B", kind: "danger"}], function(answer){})` - buttons for the
    operator; kind picks the color: ok (default), primary, danger, warn, neutral;
    `{text: "Restart", kind: "warn", restart: true}` restarts the step instead of
    calling the handler; `when: function(values){}` shows the button only while the
    condition is true - `values` are the live values of the form on screen, re-checked
    on every edit (without a form the condition sees `{}` and is checked once);
  - `step.restart()` - run the step from the beginning;
  - `step.form({"Label": field}, function(values, button){}, buttons)` - form fields, one
    per label, in the order written; a field is a text input (`"Label": "default"`), a
    drop-down (`{select: ["a", "b"], value: "a"}`) or a checkbox (`{checkbox: true}`);
    `values` holds the entered strings, a checkbox comes back as a bool. `buttons`
    (optional) replace the stock **Apply**: the same descriptors as in `step.ask`, the
    pressed one submits the form and comes to the handler as the second argument, and
    its `when(values)` condition follows the fields as the operator edits them - so a
    button shown only when every checkbox is ticked is
    `{text: "Done", when: function(v){ return v["A"] && v["B"] }}`;
  - `step.poll(function(){ return cond }, seconds, then)` - wait for a condition,
    timeout fails the step, `then` is optional (default: pass);
  - `step.info("text")` - live message under the description;
  - `step.log("text")` - note recorded into the telemetry file (a `msg` event with
    src `gcs/checklist`) while telemetry is recording; not shown in the console;
  - `step.cleanup(function(){})` - called when the step ends for any reason;
  - `step.pass(msg)` / `step.fail(msg)` - finish the step; a script that returns
    with nothing pending passes.

A step without a script shows a single Confirm button.

Blocking the takeoff while the check runs: the takeoff sequence is started by `cmd.proc.stage`,
so a guard connected to the stage change forces it back to 0 while the check is running and in
the TAKEOFF mode; it does nothing once the window is closed or the check is over, and a step
of the procedure is free to launch the aircraft after the check has passed. Declared in the
first step; the `typeof` check makes the connection once, so it survives Restart:

````
```js
if (typeof takeoffGuard === "undefined") {
    takeoffGuard = function() {
        var c = ui.checklist
        if (!c || c.phase !== "running") return
        if (cmd.proc.mode !== proc_mode_TAKEOFF) return
        if (cmd.proc.stage > 0) {
            cmd.proc.stage = 0
            c.api.log("takeoff blocked: the check is running")
        }
    }
    apx.fleet.current.mandala.cmd.proc.stage.valueChanged.connect(takeoffGuard)
}
```
````

The guard is connected to the unit selected when the step ran; after switching to another
unit the first step is to be repeated.
