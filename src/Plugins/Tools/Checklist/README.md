---
page: plugins
---

# Checklist

Aircraft check procedure driven by a Markdown file with embedded JS snippets,
shown in its own window (Tools -> Checklist).

Files in `<Documents>/UAVOS/checklist`, created with defaults on first start:

- `checklist.md` - steps. Every `## heading` is a step, `key: value` lines below the
  heading are step options (`type: auto|confirm`, `fail: stop|continue`,
  `buttons: A, B`, `wait: seconds`), fenced blocks
  ```` ```js check|start|confirm|cleanup ```` are executed by the GCS JS engine
  (same scope as the Terminal console: `mandala`, `apx`, `ins()`, `zps()`...),
  the rest of the section is Markdown shown to the operator.
- `checklist.json` - thresholds, available to snippets as the `cfg` object.

Results of every run are stored as
`<Documents>/UAVOS/checklist/reports/<timestamp>.json`.
