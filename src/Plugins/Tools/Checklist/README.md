---
page: plugins
---

# Checklist

Aircraft check procedure driven by a Markdown file with embedded JS snippets,
shown in its own window (Tools -> Checklist). Steps run strictly in order: the
current step is highlighted, only the steps already reached can be opened, none can
be skipped. Any step can be restarted: the current one with **Retry**, a passed one with
**Repeat step** (the check returns to the current step afterwards); a failed step is
retried or passed over with **Continue**. Closing the window aborts the check.

Colors, sizes and button kinds are in `ChecklistStyle.qml`, buttons and fields are
`ChecklistButton.qml` / `ChecklistField.qml`; the Markdown never styles anything.

The procedure is `Checklist.md` in the plugin resources, nothing is written to disk.
Format, nothing else is parsed:

- `# Title` - procedure title, the text below it is the description;
- a ```` ```js ```` block in the description is shared code (thresholds, helpers),
  prepended to every step script;
- `## Step` - a step, the text below it is shown to the operator;
- a ```` ```js ```` block in a step is the step script. It runs in the GCS JS engine
  (same scope as the Terminal console: `cmd.proc.mode`, `sns.pwr.vsys`, `apx`...)
  with the `step` object:
  - `step.ask(["A", {text: "B", kind: "danger"}], function(answer){})` - buttons for the
    operator; kind picks the color from `ChecklistStyle.qml`: ok (default), primary, danger, neutral;
  - `step.form({"Label": default}, function(values){})` - input fields;
  - `step.poll(function(){ return cond }, seconds, then)` - wait for a condition,
    timeout fails the step, `then` is optional (default: pass);
  - `step.info("text")` - live message under the description;
  - `step.cleanup(function(){})` - called when the step ends for any reason;
  - `step.pass(msg)` / `step.fail(msg)` - finish the step; a script that returns
    with nothing pending passes.

A step without a script shows a single Confirm button.
The runner is reachable from QML and scripts as `ui.checklist` (`state`, `takeoffAllowed`).
