---
page: plugins
---

# Signals

Configurable telemetry chart plugin for APX GCS.

## Current behavior

- Saved configurations live in `signals.json` as sets of pages and items.
- The bottom tab row uses `SignalButton`: single click switches pages, and double-click or long-press opens that page editor.
- The `+` button opens the full editor for sets, pages, items, colors, filters, and save targets.
- Pinned pages are rendered as stacked charts above the main chart inside the same widget.
- Page tabs highlight active warnings and their tooltips include the page items plus any warning messages.
- Chart overlays use `apx.font_narrow`:
  - top-right page name on every chart
  - speed label directly below the page name when the page speed is not `1x`
  - bottom-right active set name on the main chart
- Page speed is persisted per page with the allowed values `0.2x`, `0.5x`, `1x`, `2x`, `4x`.
- Speed can be changed in two places:
  - in the page editor as an enum-like selector
  - by clicking the chart itself, which cycles the page speed

## Item model

Each chart item stores:

- `bind`
- `color`
- `filters`
- `warning`
- `save`

Notes:

- Item labels are always derived from `bind`.
- Simple mandala picks are normalized to bare paths such as `est.att.roll` instead of `mandala.est.att.roll.value`.
- `save` targets use the same normalization and must stay under `sns.scr.*`.

## Filters

Filters are edited inline from the item editor, without a separate custom filter page.

Current filter types:

- `running_avg` (`Running average`)
- `kalman_smp` (`Simple Kalman`)

The filter stack is ordered and draggable. A single `Add filter` action appends a generic filter row that owns:

- an enable toggle
- a filter `type` selector
- a dynamically loaded settings section for the selected filter type

The concrete math and parameter fields live in dedicated settings facts such as `FilterRunningAverage.qml` and `FilterKalman.qml`, loaded through `FilterFact.qml`. The rendered series value and the optional `save` output both use the filtered result.

## Persistence

`SignalsModel.qml` owns persistence and legacy migration.

- New format: `{ active: { signals }, sets: [...] }`
- Legacy format accepted on load: `{ page, signalas }`
- Missing or empty settings regenerate the built-in default set

There is no hard page limit in the current editor.
