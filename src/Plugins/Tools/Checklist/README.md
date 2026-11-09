---
page: plugins
---

# Checklist

Aircraft check procedure driven by a Markdown file with embedded JS snippets.
Switched on in Tools -> Checklist -> Enabled (off by default): the window appears
and the procedure is loaded; closing the window switches the tool off again.

Steps run strictly in order: the current step is highlighted, only the steps already
reached can be opened, none can be skipped. A failed step is retried with **Retry** or
passed over with **Continue**, a passed step is repeated with **Repeat step** (the check
returns to the current step afterwards). A running step offers no restart button of its
own: the script adds one when it makes sense (`restart: true` in `step.ask`). Closing the
window aborts the check.

Files:

- `ChecklistPlugin.qml` - the plugin: Tools menu entry, procedure loading and the step
  engine (state, step API, control functions). Reachable from QML and scripts as
  `ui.checklist` (`phase`, `takeoffAllowed`);
- `ChecklistParser.js` - the Markdown parser, `parse(text)` returns
  `{title, intro, shared, steps: [{title, body, code}]}`;
- `ui/` - the window and its widgets. The window shows what the engine exposes
  (`doc`, `results`, `buttons`, `form`, `message`) and knows nothing about Markdown;
  colors and sizes are in `ui/ChecklistTheme.qml`, button kinds in `ui/ChecklistButton.qml`;
- `Checklist.md` - the procedure, in the plugin resources, nothing is written to disk.

Markdown format, nothing else is parsed:

- `# Title` - procedure title, the text below it is the description;
- a ```` ```js ```` block in the description is shared code (thresholds, helpers),
  prepended to every step script;
- `## Step` - a step, the text below it is shown to the operator;
- `{{expression}}` in the title, the description or a step text is replaced by the value
  of the JS expression, evaluated in the shared block scope when the procedure is loaded
  (`{{cfg.batteryMin}}`, `{{cfg.ers.length}}`); an error is shown in place;
- a ```` ```js ```` block in a step is the step script. It runs in the GCS JS engine
  (same scope as the Terminal console: `cmd.proc.mode`, `sns.pwr.vsys`, `apx`...)
  with the `step` object:
  - `step.ask(["A", {text: "B", kind: "danger"}], function(answer){})` - buttons for the
    operator; kind picks the color: ok (default), primary, danger, warn, neutral;
    `{text: "Restart", kind: "warn", restart: true}` restarts the step instead of
    calling the handler;
  - `step.restart()` - run the step from the beginning;
  - `step.data` - object shared by all steps of one run (`step.data.mode = answer` in one
    step, `if(step.data.mode === ...)` in a later one); cleared on **Start**;
  - `step.form({"Label": default}, function(values){})` - input fields;
  - `step.poll(function(){ return cond }, seconds, then)` - wait for a condition,
    timeout fails the step, `then` is optional (default: pass);
  - `step.info("text")` - live message under the description;
  - `step.cleanup(function(){})` - called when the step ends for any reason;
  - `step.pass(msg)` / `step.fail(msg)` - finish the step; a script that returns
    with nothing pending passes.

A step without a script shows a single Confirm button.
