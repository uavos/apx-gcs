---
page: plugins
---

# Checklist

Aircraft check procedure driven by a Markdown file with embedded JS snippets.

Procedures are the `*.md` files in `<Documents>/UAVOS/Checklists/`. The file is picked by
the callsign of the current unit: its name must be the beginning of the callsign, case
insensitive (`Borey-123` opens `borey.md`; when several names match the first one by name
is used). While a procedure exists a Checklist button is shown in the task
bar; it opens the window with it. No file, no button.

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
  Reachable from QML and scripts as `ui.checklist` (`phase`, `takeoffAllowed`) while open;
- `ChecklistParser.js` - the Markdown parser, `parse(text)` returns
  `{title, intro, steps: [{title, body, code}]}`;
- `ui/` - the window and its widgets. The window shows what the engine exposes
  (`doc`, `results`, `buttons`, `form`, `message`) and knows nothing about Markdown;
  colors and sizes are in `ui/ChecklistTheme.qml`, button kinds in `ui/ChecklistButton.qml`.

Markdown format, nothing else is parsed:

- `# Title` - procedure title, the text below it is the description;
- `## Step` - a step, the text below it is shown to the operator;
- `{{expression}}` is replaced by the value of the JS expression, evaluated in the global
  scope; an error is shown in place. The title, the description and the step titles are
  expanded when the procedure is loaded; the step body is expanded when the step becomes
  current, so it sees the values declared by the scripts of earlier steps
  (`{{cfg.batteryMin}}` works in every step after the one that declared `cfg`);
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
    calling the handler;
  - `step.restart()` - run the step from the beginning;
  - `step.form({"Label": default}, function(values){})` - input fields;
  - `step.poll(function(){ return cond }, seconds, then)` - wait for a condition,
    timeout fails the step, `then` is optional (default: pass);
  - `step.info("text")` - live message under the description;
  - `step.cleanup(function(){})` - called when the step ends for any reason;
  - `step.pass(msg)` / `step.fail(msg)` - finish the step; a script that returns
    with nothing pending passes.

A step without a script shows a single Confirm button.
