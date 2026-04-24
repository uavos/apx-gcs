# VSCode Copilot Agent Prompt — Signals Plugin Upgrade

Paste this entire prompt into your Copilot chat (Agent mode) inside the `apx-gcs` repo working tree. It describes the full feature set you will add to the existing `Signals` plugin, the reference code you should study, and concrete implementation rules.

**Starting point:** the `main` branch of `uavos/apx-gcs`. You are extending the existing `Signals` plugin in-place. There is no other branch, plugin, or prior contribution you need to consult — everything needed is described below.

**Important: do not run any git operations.** No `git add`, `git commit`, `git checkout`, `git branch`, `git mv`, `git rm`, `git rebase`, `git push`, `git stash`, `git restore`. No creating branches. No opening pull requests. Only modify the working tree. The human will handle all version control afterwards.

---

## Context for the agent

You are working in the **uavos/apx-gcs** repository (APX Ground Control Station, Qt 6 / QML / C++). The `Signals` plugin at `src/Plugins/Tools/Signals/` is a telemetry chart widget. Today it is hardcoded — fixed tab buttons `R / P / Y / Axy / Az / G / Pt / Ctr / RC / Usr / +` with fixed mandala bindings, a single global speed setting, no per-series filtering, no multiple configurations. It works, but every vehicle / airframe / mission needs its own chart setup, and operators currently have no way to save or switch those configurations.

Your job is to turn `Signals` into a configurable plugin with the same ergonomic model the `Numbers` plugin already uses elsewhere in the codebase: **sets** of **pages** of **items**, fully user-editable, persisted to JSON. On top of that, each item gets an ordered **filter stack** (e.g. running-average then Kalman) so operators can smooth noisy telemetry before plotting and, optionally, write the filtered signal back into the mandala for use by other plugins.

The end result: one plugin, one widget, no hardcoded buttons, no new dependencies, fully backward-compatible with any existing `signals.json` on disk.

### Design philosophy

- **Mirror Numbers.** The sets/pages/items editor pattern, the JSON shape, the Fact-tree wiring — all of it should look and feel identical to `Numbers`. Operators already know how Numbers works; Signals should feel like the same tool applied to charts instead of readouts.
- **Filters are plain Facts, not a custom editor page.** Model the filter stack like `datalink/ports` — an ordered, reorderable list of child Fact groups with add/remove actions, a bool on/off switch on each filter group row, and child parameter Facts shown/hidden from the selected filter `type`.
- **QML only.** No C++ changes. The plugin is QML today and will stay QML.

### Clarifications from implementation review

- **Keep the page tabs visually and behaviorally the same as the original `Signals` plugin.** Tabs only switch pages. Do not overload them with edit affordances, pin actions, long-press behavior, or extra icons.
- **The `+` button opens the full Signals editor** (sets/pages/items), same entry-point concept as `Numbers`. There is no separate sets-editor button.
- **Items do not have `title` or `act` fields.** Series labels are always derived from `bind`.
- **Normalize simple mandala paths.** If the user picks `mandala.est.att.roll.value`, store and display `est.att.roll`. Apply the same rule to `save` targets.
- **The save target behaves like the binding picker.** Use a mandala-typed Fact selector, not a duplicate free-text “save path” row.
- **Pinned charts are stacked above the main chart inside the same widget.** Pinned charts show only a top-right page-name label. The main chart shows a top-right page-name label and a bottom-right set-name label. All these overlay labels use `apx.font_narrow`.
- **There is no hard page limit.** Do not cap the number of pages per set in the editor or model.
- **There is no bottom speed button.** Clicking a chart cycles that page's speed. The speed label is shown directly below the page title and only when the page speed is not `1x`.
- **Save must be reachable from nested editor pages.** A user drilling into set/page/item/color/filter pages should still have a visible `Save` action without backing all the way out.

---

## Repository files you MUST read before writing any code

Open each of these in the editor and understand them. The two blocks below are your entire study set.

### The Signals plugin (what you will modify)

- `src/Plugins/Tools/Signals/SignalsPlugin.qml` — AppPlugin registration.
- `src/Plugins/Tools/Signals/Signals.qml` — top-level widget: page tabs, `+` editor button, chart-click speed control, pinned-chart stack, main-chart page/set labels, and the chart area. This is the file most affected by the refactor.
- `src/Plugins/Tools/Signals/SignalsView.qml` — QtCharts renderer. Handles series creation, series colors (including the desaturated rendering for `cmd.*` facts), axis auto-rescale, speed-factor timing.
- `src/Plugins/Tools/Signals/SignalButton.qml` — the authoritative tab/toggle button. Keep the page-tab interaction and visual behavior aligned with this original component.
- `src/Plugins/Tools/Signals/FilterRegistry.qml` — filter-type registry, defaults, normalization helpers.
- `src/Plugins/Tools/Signals/FilterFact.qml` — the plain Fact-based filter row used in the filter stack.
- `src/Plugins/Tools/Signals/CMakeLists.txt` — resource list; every new QML file you add must be listed here under `QRC_QML`.

### The Numbers plugin (the canonical pattern to mirror)

- `src/main/qml/Apx/Controls/numbers/NumbersModel.qml` — loads `numbers.json`, builds `NumbersItem` objects, exposes `edit()` → `NumbersMenuPopup`. This is the template for the new `SignalsModel.qml`.
- `src/main/qml/Apx/Controls/numbers/NumbersMenu.qml` — Fact-tree sets editor with `loadSettings()`/`saveSettings()`, "Add set" action, `select(num)` radio behavior, and the `json.active[settingsName]` active-index preservation logic. Template for the new `MenuSets.qml`.
- `src/main/qml/Apx/Controls/numbers/NumbersMenuSet.qml` — per-set editor with `setTitle`, a `NumbersMenuNumber` child template, and a draggable children list. Template for the new `MenuSet.qml`.
- `src/main/qml/Apx/Controls/numbers/NumbersMenuNumber.qml` — per-item editor exposing `bind`, `title`, `prec`, `warn`, `alarm`, `act`. Template for the new `MenuItem.qml`, but Signals drops `title`, `alarm`, and `act`; you will add `color`, `filters`, and `save` instead.
- `src/main/qml/Apx/Controls/numbers/NumbersMenuPopup.qml`, `NumbersItem.qml`, `NumbersBar.qml`, `NumbersBox.qml` — supporting components. Skim them; reuse their patterns rather than inventing new ones.

### Shared components

- `src/main/qml/Apx/Common/` — skim `TextButton`, `IconButton`, `ValueButton`, `FactButton`, `Style`. Reuse these; do not invent new primitives.
- Find the `datalink/ports` implementation (search under `src/Plugins/Protocols` or `src/main/qml/Apx/Menu` for `ports`). Read it carefully — it's the model for the per-item filter list: ordered, draggable, typed sub-Facts, each with an enable toggle. Note how it handles add/remove/reorder and how the typed subtree is rendered.

---

## Terminology (use these exact names in code, JSON, and UI)

- **Set** — top-level saved group, same concept as a Numbers set. One set = one complete chart configuration, typically tied to a specific system or airframe. Multiple sets can exist (e.g. `default`, `HAPS`, `R22`, …); exactly one is active at a time.
- **Page** — a named page inside a set. Each page is one tab in the widget's tab bar. Replaces today's hardcoded `R / P / Y / …` buttons.
- **Item** — one plotted variable on a page. An item has: expression (`bind`), `color`, an ordered `filters` list, an optional `warning` expression, and an optional `save` target (a mandala variable under `sns.scr.*`). The displayed series name is always derived from `bind`.
- **Filter** — one processing node inside an item's filter stack. A filter has a `type` (from an enum), an `enabled` toggle, a position in the stack (user-reorderable), and its own typed parameter subtree. Supported types at launch: `running_avg`, `kalman_smp`.

---

## Functional spec — what the upgraded `Signals` plugin must do

### 1. Sets editor (mirror Numbers exactly)

- Provide a Numbers-style sets editor reachable from the widget's `+` button (see §3 for the exact UI). Users can add / rename / delete / reorder sets; exactly one set is active at any time.
- Persist to a single JSON file in `application.prefs` named `signals.json`, using the same `{ active, sets }` structure as `numbers.json`. `active.signals` is the index of the active set. See the JSON schema section further down for the full shape.
- On first run (no `signals.json` on disk), auto-generate a **default set** that reproduces today's hardcoded Signals configuration verbatim: pages `R`, `P`, `Y`, `Axy`, `Az`, `G`, `Pt`, `Ctr`, `RC`, `Usr` with the same mandala bindings each of those hardcoded buttons currently wires up. Read the existing `Signals.qml` to extract those bindings exactly — this is how you preserve behavior for existing users.
- Provide a "Reset to defaults" action in the sets editor that regenerates the default set even if other sets already exist. The default set must never be silently deleted when the user has zero sets — in that case, auto-recreate it.
- When the active set changes, pages and items must be rebuilt from that set's contents.

### 2. Pages inside a set

- Each set holds an ordered list of pages with no hard editor cap.
- A page has: `name` (string, user-editable; suggested default when creating a new page = uppercase first character of the first item's bind, e.g. `est.att.roll` → `R`), `pin` (bool, optional — see §6), `speed` (float, per-page — see §7), and an `items` array.
- Tab buttons at the bottom of the widget show the page `name`. On hover, each tab shows a tooltip listing the page name and each item's derived label (`bind`) + color swatch, so operators can see at a glance what's on a page without switching to it.
- A warning raised by any item on a page (see §4) must visually highlight that page's tab button using the same `ValueButton.alerts` style used elsewhere, and the tooltip should include the warning message.

### 3. Page bar layout (replaces today's hardcoded button row)

The bottom bar and chart overlays should behave like this:

- **Bottom bar, left / center:** the page tabs, one per page of the active set. Keep them behaving like the original Signals plugin: clicking a tab only switches the chart page.
- **Bottom bar, right:** a `+` icon button (`IconButton { iconName: "plus" }`) that opens the full Signals editor (sets/pages/items).
- **Main chart, top-right:** the active page name, with the speed label directly below it when the page speed is not `1x`.
- **Main chart, bottom-right:** the active set name.
- **Pinned charts, top-right:** the pinned page name, with the speed label directly below it when the page speed is not `1x`.
- **Chart click:** clicking a chart cycles that page's speed.

There is no separate sets-editor button and no clickable set-name badge. The old `+` tab with its inline text-input `bind` shortcut is removed entirely.

### 4. Items inside a page

- Each item has these fields:
  - `bind` — JS expression over the mandala, required (e.g. `est.att.roll`, `cmd.rc.roll`, or a computed expression). If the user chooses a simple mandala fact from the selector, store/display it as `xxx.yyy.zzz`, not `mandala.xxx.yyy.zzz.value`.
  - `color` — hex string, optional. If empty, fall back to the palette algorithm today's `Signals.qml` uses: `Material.color(Material.Blue + i*2)` where `i` is the item index within the page. For `cmd.*` bindings keep the desaturated / lightened series rendering already in `SignalsView.addFactSeries`.
  - `filters` — ordered list of filter instances, can be empty. See §5.
  - `warning` — optional JS expression. When it evaluates truthy, the owning page tab is highlighted and the message is surfaced in the tab tooltip.
  - `save` — optional mandala variable name. Must begin with `sns.scr.` — reject anything else with a user-visible warning in the editor. This field behaves like the binding picker: it is a mandala-typed Fact selector and stores the normalized bare path.
- The per-item editor is built on the `NumbersMenuNumber` structure, but Signals does **not** expose `title` or `act`. Add `color` (via the color chooser — see §8), `filters` (inline fact children — see §5), and `save` (mandala-picker field with the `sns.scr.` guard and the dedup check described in §9). Do not add a duplicate free-text “save path” row.

### 5. Filter stack (ordered list of filters per item)

- Each item owns an ordered list of filter instances. The list can be empty (no filtering). Multiple filters run in sequence: telemetry tick → filter 0 → filter 1 → … → rendered value (and, if `save` is set, written back to the mandala).
- Model the list on `datalink/ports`: a `Filters` group Fact whose children are the filters, each draggable to reorder, each with an `enabled` toggle on the filter group row itself, and each with a typed parameter subtree decided by its `type` enum.
- Filter types at launch:
  - **`running_avg`** — single-pole running average with coefficient `coef ∈ [0, 1]`. Update rule: `out = out * (1 - coef) + in * coef`. On the first sample, initialize `out = in`. Reasonable default: `coef = 0.2`; precision / slider granularity should match `NumbersMenuNumber.prec` conventions.
  - **`kalman_smp`** — simple 1-D Kalman smoother. Parameters: `r` (measurement noise, default `0.1`) and `q` (process noise, default `0.001`). State and covariance are internal; on the first sample, seed state from the raw value and covariance to `0.1`. Standard scalar Kalman update on each subsequent sample.
- Do **not** build a separate custom filter editor page. Use plain Facts only. A filter row is a `Fact.Group | Fact.Bool` subtree with normal add/remove actions. The `type` Fact is enum-like, and the parameter Facts (`coef`, `r`, `q`, …) are ordinary child Facts shown/hidden from the selected `type`.
- The architecture must still make adding a third filter type trivial: update the filter-type registry and add the parameter subtree / runtime logic for the new type in the same plain-Fact pattern.

### 6. Pinned pages (stacked view)

- Each page has an optional `pin` boolean. When **no** pages are pinned, the widget behaves as a classic single-chart tabbed view: clicking a tab swaps the chart.
- When **one or more** pages are pinned, the widget switches to a stacked layout: every pinned page renders its own chart view, vertically stacked in a `ColumnLayout`, all visible simultaneously. Non-pinned pages remain accessible via their tabs and appear in the single remaining tabbed slot below (or above) the pinned stack — pick one placement and be consistent.
- Pinned charts must not grow extra controls. Each pinned chart shows only the chart plus a small top-right page title label.
- The main chart also shows a top-right page title label, and additionally a bottom-right active-set label.
- Use `apx.font_narrow` for all these chart overlay labels.
- Pinning / unpinning is a page-level toggle in the page editor. Do not add pin actions to the page tabs.

### 7. Speed control (per page, persisted)

- Each page has its own `speed` value, a direct float factor from the set `[0.2, 0.5, 1, 2, 4]`. This factor controls the chart timing (x-axis tick rate) — same meaning as today's speed control, just with a wider, explicit value set so long-term recording at `0.2` / `0.5` is possible.
- The page editor exposes speed as an enabled enum-like selector for the page's default speed.
- Clicking a chart cycles that page to the next speed value in the list (wrapping from `4` back to `0.2`).
- The speed label is rendered as `{value}x` (e.g. `0.5x`, `1x`, `2x`) and is shown directly below the page title only when the page speed is not `1x`.
- The current speed is persisted per page inside `signals.json` and restored on reload. Changing speed on one page does not affect any other page.
- In the pinned-stack layout, each pinned page's chart uses its own stored speed, and clicking a pinned chart cycles only that pinned page.

### 8. Colors

- Default item colors follow the palette algorithm already in `Signals.qml`: `Material.color(Material.Blue + i*2)` with `i` = item index on its page.
- Users may override per item via a color chooser. Implement the chooser as a compact 12×4 palette grid (48 Material colors) plus a hex text input for custom values. The color chooser opens from the item editor's color field.
- For `cmd.*` bindings, keep the existing desaturated / lightened rendering from `SignalsView.addFactSeries` — that rendering distinguishes commanded values from estimated values at a glance.

### 9. Save-to-mandala

- When an item has a non-empty `save`, the item's **filtered output** (post-filter-stack) is written to the named mandala variable on every tick, using the existing pattern: `apx.fleet.current.mandala.fact(fname, true).setRawValueLocal(value)`.
- The editor must enforce that `save` begins with `sns.scr.` (the `scr` = "scripting" namespace reserved for plugin-produced values). Anything else is rejected with a user-visible warning message.
- The editor must also dedup: within the active set (across all pages, all items), no two items may share the same `save` target. If the user tries to save to a name already used elsewhere, show a warning and block the save.
- The save selector must normalize simple mandala picks the same way `bind` does: `mandala.sns.scr.foo.value` is stored/displayed as `sns.scr.foo`.

### 10. Chart rendering

- Keep one chart renderer: the current `SignalsView.qml`. Adapt it to accept its data from the active set's active page (or, in pinned-stack mode, to be instanced per pinned page) rather than from hardcoded `SignalButton` children.
- Required adaptations:
  - Rebuild series when the active page / set changes, when items are added / removed / reordered, or when an item's `bind` or `color` changes. Use a `resetEnable` / dirty-flag pattern so rebuilds happen exactly once per user action.
  - Support live color edits: editing an item's color updates that series' pen without a full rebuild (an `updateSeriesColor(index, color)` method on the chart view).
  - Support the `[0.2, 0.5, 1, 2, 4]` speed factor array per-page (see §7).
  - Keep the existing auto-rescale-grow behavior (axis grows, never shrinks during a recording session).
  - For items with a non-empty filter stack, render the **filtered** value on the chart, not the raw one. The raw stream is only visible if the user explicitly adds a second item with the same `bind` and an empty filter list.
  - Preserve the original page-switch workflow driven by `SignalButton.qml`.
  - Overlay labels are owned by `Signals.qml`: top-right page label on every chart, bottom-right set label on the main chart, all using `apx.font_narrow`.

---

## Non-functional requirements

1. **No C++ changes.** Everything is QML, under `src/Plugins/Tools/Signals/` and (where you need a truly shared primitive that doesn't already exist) `src/main/qml/Apx/Common/`. If you feel a C++ change is unavoidable, stop and ask first.
2. **Backward-compatible migration.** Existing `signals.json` files may use older shapes. Specifically, any legacy file with top-level keys `page` (string) and `signalas` (array — note the typo on the old key, read it as-is) must be migrated silently on first load: wrap the legacy list into a single set named `default` with a single page whose `name` is the old `page` value (or `page 1` if missing), and whose `items` are the legacy list entries mapped field-for-field. Always write the new key `signals` (correctly spelled) on save. Never lose user data. Never show a migration dialog — the migration is invisible to the user.
3. **Localize every user-visible string with `qsTr()`.** Include the English source text inline. Match the style of the existing Signals and Numbers QML for consistency with existing translations.
4. **QML style.** 4-space indent, no tabs. Match surrounding file style. Keep `import` order consistent with sibling files. Copy the LGPL-2.1 header from `SignalsPlugin.qml` onto every new file you create.
5. **CMakeLists.** Every new QML file must be listed under `QRC_QML` in `src/Plugins/Tools/Signals/CMakeLists.txt`. Every deleted file must be removed from that list.
6. **No new third-party dependencies.** Only Qt modules already used by the project: `QtQuick`, `QtQuick.Controls`, `QtQuick.Layouts`, `QtCharts`, `QtQml.Models`, `Apx.Common`, `Apx.Controls`, `APX.Facts`, `APX.Fleet`, `APX.Mandala`.
7. **`qmllint` clean.** Every new and modified QML file must pass `qmllint` with no warnings. Report any warnings you cannot silence in your progress summary.
8. **Simulator smoke test after the upgrade.** Build the project, run GCS against the built-in simulator, and verify: (a) a fresh profile auto-generates the default set with all old Signals pages populated; (b) create a second set with two pages (one pinned, one not) and a handful of items, including at least one item with a two-filter stack `running_avg → kalman_smp` and one item with `save = sns.scr.<something>`; (c) click each chart to set a different page speed; (d) restart the app; (e) confirm all of it persisted exactly; (f) confirm the `sns.scr.*` variable is receiving filtered values visible to another plugin (e.g. Numbers).

---

## JSON schema for `signals.json`

```json
{
  "active": {
    "signals": 0
  },
  "sets": [
    {
      "title": "default",
      "pages": [
        {
          "name": "attitude",
          "pin": false,
          "speed": 1.0,
          "items": [
            {
              "bind": "est.att.roll",
              "color": "#2196F3",
              "filters": [
                { "type": "running_avg", "enabled": true, "coef": 0.2 },
                { "type": "kalman_smp",  "enabled": false, "r": 0.1, "q": 0.001 }
              ],
              "warning": "",
              "save": ""
            }
          ]
        },
        {
          "name": "power",
          "pin": true,
          "speed": 0.5,
          "items": [
            { "bind": "est.pwr.vbat", "color": "#FFC107", "filters": [], "save": "sns.scr.vbat_f" }
          ]
        }
      ]
    }
  ]
}
```

- `active.signals` is the index into `sets` of the currently active set (mirrors the `active.numbers` convention in `numbers.json`).
- Every field other than `bind` is optional; readers must tolerate missing keys and default them sensibly.
- Legacy files with top-level `page` (string) and `signalas` (array) are migrated silently on load — see non-functional requirement 2.

---

## Step-by-step plan the agent should follow

Work through these in order. Do not skip the inventory and design steps. **Do not commit anything.** Just modify files in the working tree. After each numbered step, produce a short progress summary (files added / modified / deleted + a one-sentence description) and stop for review — the human will decide when to commit.

1. **Inventory.** Read every file in `src/Plugins/Tools/Signals/` and `src/main/qml/Apx/Controls/numbers/` in full. Locate and read the `datalink/ports` implementation (search under `src/Plugins/Protocols` and `src/main/qml/Apx/Menu`). Create a short `REFACTOR_NOTES.md` inside `src/Plugins/Tools/Signals/` listing, for each existing Signals file: what it does today, what will happen to it (keep / modify / delete), and which Numbers file you will model the replacement on. This doc is working scratch — keep it updated as you go, delete it in the final cleanup step.

2. **Design sketch.** Before writing any QML, in `REFACTOR_NOTES.md`, sketch the new Fact tree for Signals: `Signals (plugin) → Sets (list) → Set → Pages (list) → Page → Items (list) → Item → Filters (list) → Filter`. Write down exactly which new QML file implements each level, which existing Numbers file it is modeled on, and which existing Signals file it replaces. Confirm the structure matches the Numbers pattern (`NumbersModel` → sets → items, with the same `active.<key>` persistence convention).

3. **Data model first.** Create `src/Plugins/Tools/Signals/SignalsModel.qml`, modeled on `NumbersModel.qml`. It owns the Fact tree (`active`, `sets`), the persistence (`loadSettings()` / `saveSettings()` reading and writing `signals.json` under `application.prefs`), and the legacy migration logic for old `{ page, signalas }` files. At this point you have no UI yet — only the model. Write a small comment block documenting the JSON schema and the migration. Verify load / save against a synthetic `signals.json` in the simulator before moving on.

4. **Default-set factory.** Inside `SignalsModel.qml` (or a sibling helper file), implement the default-set generator that is invoked when `signals.json` is missing or has zero sets. The default set must exactly reproduce today's hardcoded Signals configuration: one page per hardcoded button (`R`, `P`, `Y`, `Axy`, `Az`, `G`, `Pt`, `Ctr`, `RC`, `Usr`), each page populated with the same mandala bindings today's `Signals.qml` wires into those buttons. Extract those bindings from the current `Signals.qml` source — don't guess. Each default page starts with `pin = false`, `speed = 1.0`, and an empty `filters` list on every item.

5. **Sets / page / item editors.** Create the editor QML files:
   - `MenuSets.qml` — modeled on `NumbersMenu.qml`. Lists all sets, supports add / rename / delete / reorder, "Reset to defaults" action, persists `active.signals`.
   - `MenuSet.qml` — modeled on `NumbersMenuSet.qml`. Per-set editor: `title`, draggable pages list, "Add page" action, no page cap.
   - `MenuPage.qml` — per-page editor: `name`, `pin` toggle, `speed` (enabled enum-like selector), draggable items list, "Add item" action.
   - `MenuItem.qml` — modeled on `NumbersMenuNumber.qml`, but without `title` / `act`; extend it with `color` (opens color chooser), inline `filters`, and `save` (mandala picker with `sns.scr.` guard and dedup check).
   - `ColorChooser.qml` — 12×4 palette grid plus hex text input.
   - Ensure nested editor pages expose a visible `Save` action so the user can save while editing a set, page, item, color, or filter.

6. **Filter stack.** Implement the filter list:
   - `FilterRegistry.qml` — filter types, titles, defaults, normalization, and registry helpers.
   - `FilterFact.qml` — one plain Fact-based filter row: `Fact.Group | Fact.Bool`, enum-like `type`, child parameter Facts shown/hidden from `type`, `reset()`, `step(in)`, and a remove action.
   - The item editor owns a `Filters` group Fact that holds `FilterFact` children, supports drag reorder, and exposes an `Add filter` action.
   - Wire the filter chain into the chart pipeline: the chart series for an item with filters renders post-filter values. The filters are reset whenever the item is edited or the series is rebuilt.

7. **Widget shell.** Rewrite `Signals.qml` (and / or split it into a container + tab bar + chart area) so it renders the active set:
   - Bottom bar: original-style `SignalButton` page tabs from `sets[active].pages` and the `+` icon button on the right (opens the full editor).
   - Chart area: single `SignalsView` in the no-pins case; `ColumnLayout` of `SignalsView` instances in the pinned-pages-exist case.
   - Chart click: reads / writes the clicked page's `speed`; cycles through `[0.2, 0.5, 1, 2, 4]`.
   - Page-tab warning state: per-item `warning` expressions are evaluated and the owning tab is highlighted when truthy.
   - Overlay labels: top-right page label plus conditional speed label on the main chart, top-right page label plus conditional speed label on each pinned chart, bottom-right active-set label on the main chart. Use `apx.font_narrow`.

8. **Chart renderer adaptation.** Modify `SignalsView.qml` to take its series definitions from a page model (list of items with `bind` / `color` / filter chain) rather than from hardcoded `SignalButton` children. Add `updateSeriesColor(index, color)` for live color edits. Wire per-page `speed`. Preserve the existing `cmd.*` desaturated rendering and the auto-rescale-grow behavior. Support the save-to-mandala write on every tick for items with a non-empty `save`.

9. **Cleanup.** Keep `SignalButton.qml` as the tab interaction/style baseline. If `PageButton.qml` or other experimental tab components were added during refactor attempts and end up unused, remove them. Remove `REFACTOR_NOTES.md`. Update `src/Plugins/Tools/Signals/CMakeLists.txt`: add every new QML file, remove every deleted file. Refresh the plugin `README.md` (if present; create one if not) with a short description of the new capabilities and a pointer to the JSON schema.

10. **Build + smoke.** `cmake --build` the project. Fix any QML warnings from `qmllint`. Run the simulator smoke test described in non-functional requirement 8. If any step fails, fix it and rerun. Do not hand off with a broken build.

11. **Hand-off.** Stop. Do not create a branch, do not commit, do not open a PR. Produce a final summary containing: (a) the complete list of files added, modified, and deleted; (b) a suggested PR title and description the human can paste when they open the PR themselves (suggested title: "Signals plugin: sets / pages / items editor with filter stack"); (c) a brief screenshot / gif checklist the human should attach to the PR from the simulator smoke test.

---

## Open questions to flag

If any of these becomes a real blocker while coding, stop and ask before guessing. Otherwise default as specified.

1. **Speed scope.** The spec says speed is per page. If during implementation it becomes clear operators would prefer one speed per set, flag it. Default: per page.
2. **Legacy `signalas` typo.** Accept the old key on load, always write `signals` on save, no user-visible migration notice. Default: silent.
3. **Free-text bind shortcut.** Today's `+` tab has an inline text-input `bind` shortcut. The full Signals editor replaces it. If, during smoke testing, it turns out the quick inline entry is actually useful for operators, flag it as a small follow-up — don't add it back unprompted. Default: remove.

---

## Constraints on your behavior

- **No new third-party dependencies.** Only Qt modules already referenced by the project.
- **No unrelated file changes.** Exception: if you notice an obvious typo (a comment misspelling, a stale TODO) *in a file you are already editing for this task*, fix it inline. Do not go typo-hunting through the rest of the codebase.
- **No C++ signature changes** anywhere — not in `PluginInterface`, `Fact`, `AppRoot`, or any other C++ file. This is a QML-only task.
- **Ask only on real conflicts.** Do not ask for permission for obvious style, naming, or micro-layout choices — pick the one that matches the surrounding code.
- **Terse progress updates.** After each of the 11 steps, report: files touched, approximate lines added / removed, one-sentence summary of what now works. No essays, no recaps of the prompt.
- **No git operations, at all.** No `git add`, `git commit`, `git checkout`, `git branch`, `git mv`, `git rm`, `git rebase`, `git cherry-pick`, `git push`, `git stash`, `git restore`. Renames are plain filesystem copy-then-delete. Deletions are plain `rm`. The only acceptable git usage is **read-only inspection** (`git log`, `git diff`) to orient yourself in the repo.
- **`qmllint` clean, step by step.** Run `qmllint` on every new or modified QML file as you go, not just at the end. Report warnings in the step summary. Do not worry about committing — the human handles that.
