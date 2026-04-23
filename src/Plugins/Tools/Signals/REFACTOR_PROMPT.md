## Context for the agent

You are working in the **uavos/apx-gcs** repository (APX Ground Control Station, Qt 6 / QML / C++). The active branch is `FiltredCharts`. PR #111 ("Filtred charts") by SokolovskyYury adds a new plugin at `src/Plugins/Tools/FilteredCharts/`. The owner/architect (Aliaksei Stratsilatau, @uavinda) has decided, after discussion with the field operators, that this plugin as-submitted will not ship. Instead, you will **refactor the existing `Signals` plugin in-place** so that it natively absorbs the FilteredCharts functionality. The goal: one plugin, one set of buttons, Miller's-rule-friendly UI, backward-compatible with the current `Signals` layout and behavior.

**Do not create a second plugin.** The `FilteredCharts` directory and PR will be superseded by enhancements to `Signals`. Once the refactor is complete, the `FilteredCharts` plugin directory must be removed from `src/Plugins/Tools/` and from the build.

### Source of the requirements

The spec below is the distillation of the architect's design decisions posted in Discord `#gcs` on 2026-04-23. Every point is a hard requirement unless marked optional. Do not reinterpret or drop any of them without asking.

---

## Repository files you MUST read before writing any code

Read these in full and understand the relationships. Open each one in the editor.

### Current FilteredCharts plugin (to be absorbed, then deleted)
- `src/Plugins/Tools/FilteredCharts/FilteredChartsPlugin.qml` — AppPlugin registration
- `src/Plugins/Tools/FilteredCharts/FilteredCharts.qml` — top-level widget: page buttons 1..10, speed button, chart area, settings persistence to `charts.json`
- `src/Plugins/Tools/FilteredCharts/FcChartsView.qml` — QtCharts renderer; near-duplicate of `SignalsView.qml` with `speedFactor=[0.2,0.5,1,2,4]`, per-series `updateSeriesColor()`, `resetEnable` flag
- `src/Plugins/Tools/FilteredCharts/FcButton.qml` — per-page button; owns an `FcMenuSet`, has `getSet()/loadSet()/setSpeed()`, reacts to `mandala.onTelemetryDecoded`
- `src/Plugins/Tools/FilteredCharts/FcMenuSet.qml` — Fact-tree editor for a set: title, speed, `msValues` group with `FcMenuChart` children, Save action
- `src/Plugins/Tools/FilteredCharts/FcMenuChart.qml` — per-chart editor: `chartname`, `bind` (expression), color, filters, `save` target; implements `useRunningAvgFilter()`, `useKalmanSmpFilter()`, state/covariance
- `src/Plugins/Tools/FilteredCharts/FcMenuFilters.qml` — filter picker enum `["none", "running_avg", "kalman_smp"]` containing `FcFilterRunningAvg` + `FcFilterKalmanSimple`
- `src/Plugins/Tools/FilteredCharts/FcFilterRunningAvg.qml` — coefficient K ∈ [0,1]
- `src/Plugins/Tools/FilteredCharts/FcFilterKalmanSimple.qml` — measurement noise + environment noise
- `src/Plugins/Tools/FilteredCharts/FcMenuColor.qml` — color Fact routed to `FcColorChooser.qml` page
- `src/Plugins/Tools/FilteredCharts/FcColorChooser.qml` — 12×4 palette grid
- `src/Plugins/Tools/FilteredCharts/CMakeLists.txt`

### Current Signals plugin (the base you will refactor)
- `src/Plugins/Tools/Signals/SignalsPlugin.qml`
- `src/Plugins/Tools/Signals/Signals.qml` — hardcoded buttons R / P / Y / Axy / Az / G / Pt / Ctr / RC / Usr / `+` (custom text input) + speed button
- `src/Plugins/Tools/Signals/SignalsView.qml` — QtCharts renderer
- `src/Plugins/Tools/Signals/SignalButton.qml` — minimal toggle button
- `src/Plugins/Tools/Signals/CMakeLists.txt`

### Numbers plugin — the canonical set/editor pattern to mirror
- `src/main/qml/Apx/Controls/numbers/NumbersModel.qml` — loads `numbers.json`, builds `NumbersItem` objects, exposes `edit()` → `NumbersMenuPopup`
- `src/main/qml/Apx/Controls/numbers/NumbersMenu.qml` — Fact-tree set editor, `loadSettings()/saveSettings()`, "Add set" action, `select(num)` radio behavior, preserves `json.active[settingsName]`
- `src/main/qml/Apx/Controls/numbers/NumbersMenuSet.qml` — per-set editor with `setTitle`, `NumbersMenuNumber` template, drag-children values list
- `src/main/qml/Apx/Controls/numbers/NumbersMenuNumber.qml` — per-item editor: `bind`, `title`, `prec`, `warn`, `alarm`, `act`
- `src/main/qml/Apx/Controls/numbers/NumbersMenuPopup.qml`
- `src/main/qml/Apx/Controls/numbers/NumbersItem.qml`
- `src/main/qml/Apx/Controls/numbers/NumbersBar.qml`
- `src/main/qml/Apx/Controls/numbers/NumbersBox.qml`

Also skim `src/main/qml/Apx/Common/` for `TextButton`, `IconButton`, `ValueButton`, `FactButton`, `Style` — reuse these; do not invent new primitives.

Also skim how `datalink/ports` is modeled as a dynamic list (for the multi-filter list pattern). Search under `src/Plugins/Protocols` or `src/main/qml/Apx/Menu` for `ports` to find it.

---

## Terminology (use these exact names in code and JSON)

- **Set** — top-level saved group, same concept as a Numbers set. One set = one full chart configuration tied to a specific system/airframe. There can be many sets (e.g. "default", "HAPS", "R22", …). Only one set is active.
- **Page** — a named page inside a set, corresponds to one of the tabs at the bottom of the widget (today: R, P, Y, Axy, Az, G, Pt, Ctr, RC, Usr, +). The default set's pages must be identical to the hardcoded Signals pages.
- **Item** — a single plotted variable (previously a `SignalButton.values[i]` entry or an `FcMenuChart` entry). Each item has: expression (`bind`), optional `title`, `color`, per-item `filters` (ordered list, can be empty, can have multiple), optional `warning`/`alarm`/`act`, optional `save` target (`sns.scr.*`).
- **Filter** — a processing node inside an item's filter chain. Types at minimum: `running_avg`, `kalman_smp`. Model it as a list (like `datalink/ports`) so more filter types can be added later.


---

## Functional spec — what the refactored `Signals` plugin must do

### 1. Unify with FilteredCharts
- The plugin is `Signals`, located at `src/Plugins/Tools/Signals/`. No new plugin directory.
- `FilteredCharts` plugin directory, its entry in `src/Plugins/Tools/CMakeLists.txt`, and the `FilteredChartsPlugin` registration must be removed at the end of the refactor.
- Any QML helper worth keeping from `FilteredCharts/` (chart view, color chooser, filter facts) must be **moved** into the `Signals` plugin directory with appropriate renaming (e.g. `ChartsView.qml`, `ColorChooser.qml`, `FilterRunningAvg.qml`, `FilterKalmanSimple.qml`, `MenuFilters.qml`, `MenuColor.qml`, `MenuChart.qml`, `MenuSet.qml`). The copyright header must be preserved on every copied file. Drop the `Fc` prefix.

### 2. Sets editor (reuse Numbers pattern)
- Provide a Numbers-style sets editor. A user can create/rename/delete sets; one set is active at a time. Persist to a single JSON file in `application.prefs`, e.g. `signals.json`. Use the same `json.active`/`json.sets` structure as `numbers.json`.
- Default set is auto-generated on first run and reproduces today's hardcoded `Signals` configuration byte-for-byte: pages `R, P, Y, Axy, Az, G, Pt, Ctr, RC, Usr` with the exact same mandala bindings. The default set must always be regenerable (provide a "Reset to defaults" action). Do not delete the default if the user has no other sets.
- When the active set changes, pages and items are rebuilt from that set.

### 3. Pages inside a set
- Each set contains an ordered list of pages (max 10 — keep current limit, Shpilevski's HAPS requirement).
- A page has: `name` (string, user-editable; default = first character of the first item's variable, uppercase — e.g. `est.att.roll` → `R`), `pin` (bool, optional — see #6), and an `items` array.
- The `+` button (top-right, where `IconButton { iconName: "plus" }` sits today in `FilteredCharts.qml`) opens the currently active page for editing. It is not a tab any more — replaced by the set name label (see next point).
- The bottom-right of the bar shows the **set name** as a plain label (or short dropdown to switch sets), not a `+` tab. Clicking it opens the sets editor.
- Tab buttons show the page `name` (not 1..10). On hover show a tooltip listing the page name and its items with their colors — the existing `updateToolTip()` logic in `FcButton.qml` is the right model.

### 4. Items inside a page
- Each item has: `bind` (JS expression, required), `title` (optional), `color` (hex, optional — falls back to palette like Signals does today via `Material.color(Material.Blue+i*2)`), `filters` (ordered list, can be empty), `warning` (optional JS expression), `alarm` (optional JS expression), `act` (optional JS action), `save` (optional mandala var, must start with `sns.scr` — reuse check from `FcMenuChart.saveValue2Fact()`).
- Per-item editor reuses `NumbersMenuNumber` structure — same fields, same `load()/save()/settingName()/updateTitle()/updateDescr()` conventions — plus `color` and `filters`.
- When `warning` or `alarm` expressions evaluate true, highlight the **page button** (the tab) the same way `NumbersItem` highlights itself (use `ValueButton.alerts`), and show the warning message + tooltip on the page button. This is the "warning from `numbers` in item, highlight the `page` button" requirement.

### 5. Filters as an ordered list
- Replace the current single-enum filter selector with a **list** of filter instances per item, modeled after `datalink/ports`. Each filter is a Fact with a `type` (enum), a position (draggable), an enable toggle, and its own parameter subtree.
- Supported filter types at launch: `running_avg` (reuse `FcFilterRunningAvg.qml` logic) and `kalman_smp` (reuse `FcFilterKalmanSimple.qml` logic). Architecture must make it trivial to add a new type — one new QML file + one entry in the enum registry.
- At runtime, `updateValue()` runs the filters sequentially on each telemetry tick: output of filter N is input to filter N+1.
- The Kalman filter must still seed its state/covariance from the first raw value (keep the `setKalmanState(v, 0.1)` behavior).

### 6. Pinned pages
- Add a per-page `pin` option. When one or more pages are pinned, the widget shows them stacked (several chart views in the same plugin panel). Non-pinned pages remain a single-slot tabbed view. Implement this as multiple `ChartsView` instances vertically stacked in a `ColumnLayout` when `pages.filter(p => p.pin).length > 0`. Tab buttons for pinned pages visually indicate their pinned state (e.g. outlined border, or a pin icon overlay).

### 7. Speed / scale button
- Fix the bug noted in review: the speed button in `SignalsView.qml` uses the index-based `speedFactor` array and does not persist; the one in `FcChartsView.qml` uses direct factor values but the per-set Speed slider in `FcMenuSet.qml` writes into a fact that isn't wired back on reload in all cases. Pick **one** model: per-page speed, direct factor value, options `[0.2, 0.5, 1, 2, 4]` (keep FilteredCharts' set, since 0.2 and 0.5 are needed for long-term recording). Persist speed per page. Clicking the speed button cycles to the next value. Label remains `{value}x`.

### 8. Colors
- Default item colors follow the existing `Signals` algorithm: `Material.color(Material.Blue + i*2)` where `i` is the item index within the page. The user may override per item via the color chooser (reuse `FcColorChooser.qml`, rename to `ColorChooser.qml`). For legacy `cmd.*` items keep the desaturated/lightened rendering from `FcChartsView.addFactSeries` / `SignalsView.addFactSeries`.

### 9. Save-to-mandala
- Keep the `save` field with the `sns.scr.*` guard (`chartWarning` when the user picks a non-`sns.scr` name). When an item has a `save` target, the filtered output of that item is pushed into the mandala variable via `apx.fleet.current.mandala.fact(fname, true).setRawValueLocal(value)` on every tick — identical to `FcMenuChart.saveValue2Fact()`. Also keep the `fcControl.checkScrMatches(text)` dedup check across all items in all pages of the active set.

### 10. Chart rendering
- Use one `ChartsView.qml` (renamed from `FcChartsView.qml`) as the renderer. Delete `SignalsView.qml` — its functionality is a strict subset. Port over the FilteredCharts improvements: `resetEnable` on facts change, `updateSeriesColor()` for live color edits, instant rescale-grow, and the `speedFactor=[0.2,0.5,1,2,4]` array.


## Non-functional requirements

1. **No C++ changes unless strictly required.** All work should be in QML under `src/Plugins/Tools/Signals/` and (where shared) `src/main/qml/Apx/Common/`. The plugin is QML-only today and should stay that way.
2. **Preserve the `FactButton.qml` `iconColor` change** introduced by PR #111 — that is a real improvement and is already merged conceptually; keep it in `src/main/qml/Apx/Common/FactButton.qml`.
3. **Revert `.vscode/settings.json`** if the FiltredCharts branch added private IDE settings there — keep the file clean.
4. **Backward-compatible migration.** Old `signals.json` files used a flat `{page: "string", signalas: [...]}` shape (note Yury's typo `signalas` — keep reading it, write as `signals`). On first load, if the old shape is detected, migrate to the new `{active, sets}` shape by wrapping the legacy list into a single set called "default" with a single page called (old `page` value or "page 1"). Never lose user data.
5. **Localize every user-visible string with `qsTr()`**. Include English source text. Match the style of existing Signals/Numbers QML.
6. **QML style.** 4-space indent, no tabs. Match the surrounding file style. Keep `import` order consistent with sibling files. Keep LGPL-2.1 headers on every new file (copy the header from `SignalsPlugin.qml`).
7. **CMakeLists.** Update `src/Plugins/Tools/Signals/CMakeLists.txt` to add any new QML files under `QRC_QML`. Remove `FilteredCharts` from `src/Plugins/Tools/CMakeLists.txt` (the `add_subdirectory(FilteredCharts)` line added by PR #111 — if it was added — must go).
8. **No new third-party dependencies.** Only Qt modules already used by the project (`QtQuick`, `QtQuick.Controls`, `QtQuick.Layouts`, `QtCharts`, `QtQml.Models`, `Apx.Common`, `Apx.Controls`, `APX.Facts`, `APX.Fleet`, `APX.Mandala`).
9. **Simulator smoke test** after the refactor: run GCS against the built-in simulator, create a set with two pages (one pinned, one not), add items with and without filters, toggle the speed button, edit a color, save to `sns.scr.*`, restart the app, confirm the state persisted.

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
              "title": "roll",
              "color": "#2196F3",
              "filters": [
                { "type": "running_avg", "enabled": true, "coef": 0.2 },
                { "type": "kalman_smp",   "enabled": false, "r": 0.1, "q": 0.001 }
              ],
              "warning": "",
              "alarm": "",
              "act": "",
              "save": ""
            }
          ]
        },
        {
          "name": "power",
          "pin": true,
          "speed": 0.5,
          "items": [
            { "bind": "est.pwr.vbat", "title": "Vbat", "color": "#FFC107", "filters": [], "save": "sns.scr.vbat_f" }
          ]
        }
      ]
    }
  ]
}
```

- `active.signals` = index into `sets` of the currently visible set (mirrors `NumbersModel`'s `active.numbers`).
- Every field other than `bind` is optional. Readers must tolerate missing keys.
- Legacy (pre-refactor) files with keys `page` (string) and `signalas` (array) must be migrated silently — see non-functional requirement 4.

## Step-by-step plan the agent should follow

Work in this order. Commit after each numbered step with a clear message. Do not skip the inventory and design steps.

1. **Inventory.** Read every file in `src/Plugins/Tools/FilteredCharts/` and `src/Plugins/Tools/Signals/` and `src/main/qml/Apx/Controls/numbers/`. Write a short `REFACTOR_NOTES.md` in the Signals plugin dir listing every Fc* component, what it does, and which Signals/Numbers component it maps to in the new world. This doc is your reference — keep it updated as you go and delete it in the final cleanup step.
2. **Design sketch.** Before writing code, in `REFACTOR_NOTES.md`, sketch the new Fact tree for Signals: `Signals (plugin) → Sets (list) → Set → Pages (list) → Page → Items (list) → Item → Filters (list) → Filter`. Map each level to a QML file. Confirm it matches the Numbers pattern (`NumbersModel` → `sets` → `pages` → `items`).
3. **Scaffold with renames.** Copy each `Fc*.qml` to its new name inside `Signals/` and strip the `Fc` prefix: `FcChartsView.qml` → `ChartsView.qml`, `FcButton.qml` → `PageButton.qml`, `FcMenuSet.qml` → `MenuSet.qml`, `FcMenuChart.qml` → `MenuItem.qml`, `FcMenuFilters.qml` → `MenuFilters.qml`, `FcFilterRunningAvg.qml` → `FilterRunningAvg.qml`, `FcFilterKalmanSimple.qml` → `FilterKalmanSimple.qml`, `FcMenuColor.qml` → `MenuColor.qml`, `FcColorChooser.qml` → `ColorChooser.qml`. Update every import and type reference. Delete the old `Signals.qml`, `SignalsView.qml`, `SignalButton.qml` — they are superseded. At this point the plugin should build and show the old FilteredCharts UI under the name "Signals".
4. **Data model.** Create `SignalsModel.qml` mirroring `NumbersModel.qml` (sets, active index, persistence through `Fact` + `defaults` + JSON). Wire `signals.json` load/save. Implement the legacy migration for the `page`/`signalas` shape.
5. **Default-set factory.** Build the default first-run set so a fresh user sees useful content: one set "default" with pages `attitude` (roll/pitch/yaw), `rates` (Ax/Ay/Az), `gyro`, `pitot`, `ctr`, `rc`, `user` — matching the hardcoded buttons in today's `Signals.qml`. These are seeded only when `signals.json` is missing or empty.
6. **Tabs, pin, speed.** In the main widget (rename `Signals.qml`'s role into `SignalsView.qml`-style container), render page tabs from the active set. Replace the old `+` tab with a set-name label at bottom-right (click → sets editor) and a `+` button at top-right (click → active page's item editor). Implement the pinned-pages stacked layout. Wire the per-page speed button.
7. **Item editor.** Build `MenuItem.qml` on the `NumbersMenuNumber` template: same field conventions, same `descr` wiring, same Apply/Cancel behavior. Add `color` (via `ColorChooser`), `filters` (list editor), `save` (with `sns.scr` guard + dedup).
8. **Filter list.** Implement filters as an ordered list of Facts under each item, modeled on `src/Plugins/System/DataLink/ports` (draggable, enable toggle, typed subtree). Register the two built-in filter types. Document in `REFACTOR_NOTES.md` exactly how to add a third filter type.
9. **Cleanup.** Delete `src/Plugins/Tools/FilteredCharts/` entirely. Remove its `add_subdirectory` from `src/Plugins/Tools/CMakeLists.txt`. Remove `REFACTOR_NOTES.md`. Update `src/Plugins/Tools/Signals/README.md` with a short summary of the new capabilities.
10. **Build + smoke.** `cmake --build` the project. Fix any QML warnings from `qmllint` on the new files. Run the simulator smoke test described in non-functional requirement 9.
11. **Pull request.** Create a new branch `signals-unified` off `main` (do **not** rebase on `FiltredCharts`). Cherry-pick the `FactButton.qml` `iconColor` change from PR #111 so that contribution is preserved. Open PR titled "Signals plugin: unified sets/pages/filters (replaces PR #111)". In the PR description, link PR #111 and say it should be closed in favor of this one. List the files deleted, added, and modified. Include a screenshot/gif of the simulator smoke test.

## Open questions to flag

If any of these becomes a real blocker while coding, stop and ask before guessing. Otherwise default as specified.

1. **Speed scope.** The spec says speed is per page. If during implementation it becomes obvious users want one speed per set instead, flag it. Default: per page.
2. **Legacy `signalas` typo.** Keep reading the old key on load but always write `signals` on save. No user-visible migration message needed.
3. **The old `+` free-text item input** in `Signals.qml` (the row with `TextField` that let users type arbitrary `bind` strings inline) — is it worth keeping as a shortcut next to the item editor? Default: remove it; the full item editor replaces it.

## Constraints on your behavior

- **No new third-party dependencies.** Qt modules already referenced by the project only.
- **No unrelated file changes.** Exception: if you notice the `PligIns` typo in `src/main/AppRoot.cpp` comments or similar obvious typos *in files you are already editing*, fix them. Do not go typo-hunting.
- **No C++ signature changes** in `PluginInterface`, `Fact`, `AppRoot`, etc. QML-only refactor.
- **Ask only on real conflicts.** Do not ask for permission for obvious style or naming choices — pick the one that matches the surrounding code.
- **Terse progress updates.** After each of the 11 steps, report: files touched, lines added/removed, one-sentence summary. No essays.
- **Preserve git history where reasonable.** Use `git mv` for renames so blame is preserved. Don't rewrite existing commits.
- **Run `qmllint`** on every new and modified QML file before committing the step.

## Reference — Discord context (2026-04-23)

### Architect's decision (Aliaksei, @uavinda), `#gcs`, 12:59 MSK

> If we add a pages editor to `Signals`, and give each chart the ability to enable filters, we end up with `FilteredCharts`. So let's not multiply entities — do it all inside `Signals` and drop `FilteredCharts`.
>
> What's needed:
> - In `Signals`, remove the hardcoded R/P/Y/Axy/Az/G/Pt/Ctr/RC/Usr buttons — replace them with a sets/pages editor like in `Numbers`.
> - Each chart item has: bind, color, filters (ordered list, multiple can run in sequence), warning/alarm/act, optional save to `sns.scr.*`.
> - Pages can be pinned — pinned pages are shown stacked.
> - Speed button is per page, values `[0.2, 0.5, 1, 2, 4]`, persisted.
> - A warning from an item highlights its page tab.
> - Everything persists in `signals.json` with the `{active, sets}` structure, same as `Numbers`.
> - Close PR #111, open a new PR off `main`.

### UI review (Slava Vasiukovich), `#gcs`, 11:00 MSK

1. Functional duplication with the existing `Signals` plugin — no reason for a second plugin with a similar widget.
2. The scale/speed button behaves differently from neighboring plugins — different values, different persistence logic.
3. Tab names "1..10" mean nothing to the operator — they need meaningful names.

### Counter (Yury, author of PR #111)

Yury argued the plugin is distinct because of the filter chain. Aliaksei's resolution: filter chain moves into Signals, plugin merges, PR #111 is closed.

### PR

- [PR #111 — "Filtred charts"](https://github.com/uavos/apx-gcs/pull/111) by SokolovskyYury — 18 files, +1638 / -88. To be closed when `signals-unified` merges.

### FiltredCharts branch commits to cherry-pick ideas from (most recent first)

- `c72cdab` Change speed with a button click
- `3c41e3f` Charts view refactoring
- `f66ca99` Button minor fix
- `737db79`, `3904baa`, `fa708d7` Chart/color menu fixes
- `681a715` Set running avg defaults coef
- `ef2f9ce`, `16bc4b3`, `28733ef`, `32e10d1`, `bf8b423` Various fixes
- `d90d99a` Similar names fix
- `8a414c6` Used scripts var check
- `32a3f06`, `8fc0746` newItem icon + pinned menu fixes
- `fda3b2e`, `d69f514`, `983341d`, `ee6c6c5`, `b31ed47` Chart reset/color/apply fixes
- `d1a3f5a` Coefs range and precision
- `7ecae37` Color menu added
- `4c5290d` Write values without sending implemented
- `d5a1c9e`, `b6e1ef5`, `a689078`, `0cf8c9f`, `2f7e8f9` Refactoring, saving, save-to-fact

The `FactButton.qml` `iconColor` change from this branch is worth cherry-picking into the new PR.
