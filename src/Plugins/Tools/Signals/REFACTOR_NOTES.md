# Signals Refactor Notes

Current implementation snapshot for `src/Plugins/Tools/Signals/`.

## Runtime architecture

- `SignalsPlugin.qml`
  - Registers the plugin and mounts `Signals {}`.
- `Signals.qml`
  - Hosts `SignalsModel`.
  - Renders original-style `SignalButton` page tabs.
  - Opens the full editor from the `+` button.
  - Renders pinned charts above the main chart.
  - Cycles page speed when the user clicks a chart.
  - Shows chart overlays with `apx.font_narrow`:
    - top-right page title on all charts
    - speed label below the page title only when speed is not `1x`
    - bottom-right active set label on the main chart
- `SignalsView.qml`
  - Owns the chart renderer.
  - Builds series from page items.
  - Reads raw samples from live page items.
  - Delegates filter execution to each item's `updateFilters()` chain.
  - Writes filtered values to `sns.scr.*` save targets.
  - Preserves `cmd.*` desaturated styling and grow-only Y-axis behavior.
- `SignalButton.qml`
  - Remains the active tab-button implementation.
  - Single click switches pages; double-click / long-press opens that page editor.
  - Surfaces warning state through tab color and tooltip content.

## Persistence and model

- `SignalsModel.qml`
  - Owns `signals.json` load/save.
  - Persists `{ active: { signals }, sets: [...] }`.
  - Migrates legacy `{ page, signalas }` on load.
  - Regenerates the built-in default set when settings are missing or empty.
  - Exposes page-speed helpers and page mutation helpers.

Current page/item schema:

```json
{
  "title": "default",
  "pages": [
    {
      "name": "R",
      "pin": false,
      "speed": 1.0,
      "items": [
        {
          "bind": "est.att.roll",
          "color": "",
          "filters": [],
          "warning": "",
          "save": ""
        }
      ]
    }
  ]
}
```

Notes:

- There is no hard page limit in the current editor.
- Simple mandala paths are normalized to bare paths such as `est.att.roll`.
- The same normalization is applied to `save` targets.

## Editor tree

- `MenuSets.qml`
  - Root sets editor.
  - Add/remove/reorder/select sets.
  - Reset to defaults.
  - Save the full chart configuration.
- `MenuSet.qml`
  - One set editor.
  - Set name, page list, add/remove pages, save action.
  - No page-limit enforcement.
- `MenuPage.qml`
  - One page editor.
  - Page name, pin toggle, editable enum-like speed selector, item list.
- `MenuItem.qml`
  - One item editor.
  - No `title` field.
  - No `act` field.
  - Fields: binding, color, filters, warning, save target.
  - Nested save actions stay visible while editing.
- `ColorChooser.qml`
  - 12x4 Material palette plus hex input.
- `FilterFact.qml`
  - Generic filter row with enable toggle, type selector, save/remove actions, and a dynamic settings section.
- `FilterBase.qml`
  - Shared base for the type-specific settings Fact loaded inside `FilterFact.qml`.
- `FilterRunningAverage.qml`, `FilterKalman.qml`
  - Dedicated filter settings Facts; each file owns its own editor fields and runtime math.
- `FilterRegistry.qml`
  - Filter defaults, titles, type normalization, and component lookup.

## Filter implementation

Current filter types:

- `running_avg`
- `kalman_smp`

Current pattern:

- Filters are generic `FilterFact.qml` rows inside the item’s `Filters` group.
- One generic `Add filter` action appends a new wrapper row with the registry default type.
- Each filter is ordered and draggable.
- Each filter has an enable toggle on the row itself.
- Each filter row owns the enable/type shell and swaps the loaded settings Fact when the type changes.
- Each loaded settings Fact owns the runtime state and `update()` implementation.
- `MenuItem.qml` applies the live filter chain; `SignalsView.qml` only provides raw samples.

## Current UX decisions

- `+` opens the full Signals editor, not a quick-bind field.
- Single click on a tab switches pages.
- Double-click or long-press on a tab opens that page editor.
- Pin actions remain in the page editor; there is no tab pin control.
- Pinned pages are controlled from the page editor.
- Page-tab tooltips list item labels and active warning messages.
- Warning state colors the affected tab.
- Speed is per page.
- The dedicated bottom speed button was removed.
- Clicking a chart cycles that page’s speed.
- The speed label is only shown when the page speed is not `1x`.
- Save targets must stay under `sns.scr.*` and are deduplicated across the active set.
- Only the warning expression is kept. There is no separate alarm field.

## File status

| File | Current role | Status |
| --- | --- | --- |
| `Signals.qml` | Runtime shell and overlays | active |
| `SignalsModel.qml` | persistence and runtime helpers | active |
| `SignalsView.qml` | chart renderer and filter execution | active |
| `SignalButton.qml` | page tabs | active |
| `MenuSets.qml` | sets editor root | active |
| `MenuSet.qml` | set editor | active |
| `MenuPage.qml` | page editor | active |
| `MenuItem.qml` | item editor | active |
| `ColorChooser.qml` | color editor page | active |
| `FilterFact.qml` | generic filter shell | active |
| `FilterBase.qml` | shared base for filter settings facts | active |
| `FilterRunningAverage.qml` | running average settings + runtime | active |
| `FilterKalman.qml` | Kalman settings + runtime | active |
| `FilterRegistry.qml` | filter metadata and normalization | active |
| `PageButton.qml` | old experiment from an earlier tab rewrite | currently unused |

## Documentation sync

The markdown files should reflect these current decisions:

- no page limit
- no bottom speed button
- chart click cycles speed
- speed label sits below the page title and is hidden at `1x`
- `SignalButton.qml` remains the active tab path
- tab double-click / long-press opens the page editor
- page tabs surface warning state and warning text
- item schema is `bind/color/filters/warning/save`
- filters use a generic wrapper row with a type selector that swaps dedicated settings facts
