# Signals Plugin Refactor Notes

## Source Analysis

### FilteredCharts components → new name in Signals/

| Fc* file | New name | Role |
|---|---|---|
| FcChartsView.qml | ChartsView.qml | QtCharts renderer (replaces SignalsView.qml) |
| FcButton.qml | PageButton.qml | Per-page tab button |
| FcMenuSet.qml | MenuSet.qml | Per-page Fact-tree editor (now per-set) |
| FcMenuChart.qml | MenuItem.qml | Per-item editor |
| FcMenuFilters.qml | MenuFilters.qml | Filter selector per item |
| FcFilterRunningAvg.qml | FilterRunningAvg.qml | Running-avg filter params |
| FcFilterKalmanSimple.qml | FilterKalmanSimple.qml | Kalman filter params |
| FcMenuColor.qml | MenuColor.qml | Color fact that points to ColorChooser page |
| FcColorChooser.qml | ColorChooser.qml | 12×4 palette grid |
| FilteredCharts.qml | → absorbed into Signals.qml | Top-level widget |
| FilteredChartsPlugin.qml | → deleted | Plugin registration not needed |

### Signals components → fate

| Signals file | Fate |
|---|---|
| SignalsPlugin.qml | Keep (is the plugin registration) |
| Signals.qml | Rewrite — new top-level widget |
| SignalsView.qml | Delete — ChartsView.qml is a superset |
| SignalButton.qml | Delete — PageButton.qml replaces it |

---

## Fact Tree Design

```
Signals (plugin, Rectangle root)
  SignalsModel (ObjectModel — loads signals.json)
    ┌─ [active index = json.active.signals]
    └─ sets: array
         └─ SignalsMenu (Fact, like NumbersMenu)
              ├─ MenuSet #0 (Fact.Group+FlatModel)  ← active set
              │    ├─ title
              │    └─ pages: MenuPage #0..N
              │         ├─ name (string)
              │         ├─ pin (bool)
              │         ├─ speed (float [0.2,0.5,1,2,4])
              │         └─ items: MenuItem #0..M
              │              ├─ bind (expression)
              │              ├─ title (optional)
              │              ├─ color (hex)
              │              ├─ MenuFilters
              │              │    ├─ FilterRunningAvg
              │              │    └─ FilterKalmanSimple
              │              ├─ warn
              │              ├─ alarm
              │              ├─ act
              │              └─ save (sns.scr.* target)
              └─ MenuSet #1 …
```

Mapping to Numbers pattern:
- `NumbersModel` → `SignalsModel` (ObjectModel, loads signals.json, exposes `edit()`)
- `NumbersMenu` → `SignalsMenu` (Fact, set editor, save/load signals.json)
- `NumbersMenuSet` → `MenuSet` (per set; has pages instead of values)
- `NumbersMenuNumber` → `MenuItem` (per item; adds color + filters + save)

---

## JSON Schema

```json
{
  "active": { "signals": 0 },
  "sets": [
    {
      "title": "default",
      "pages": [
        {
          "name": "R",
          "pin": false,
          "speed": 1.0,
          "items": [
            { "bind": "est.att.roll", "title": "", "color": "", "filters": [], "warn": "", "alarm": "", "act": "", "save": "" }
          ]
        }
      ]
    }
  ]
}
```

Legacy migration: if file has top-level `page` (string) + `signalas` (array), wrap into one set "default" with one page.

---

## Default Set — Binding Map

Matches today's hardcoded Signals.qml buttons:

| Page name | Items (bind expressions) |
|---|---|
| R | cmd.att.roll, est.att.roll |
| P | cmd.att.pitch, est.att.pitch |
| Y | cmd.pos.bearing, cmd.att.yaw, est.att.yaw |
| Axy | est.acc.x, est.acc.y |
| Az | est.acc.z |
| G | est.gyro.x, est.gyro.y, est.gyro.z |
| Pt | est.pos.altitude, est.pos.vspeed, est.air.airspeed |
| Ctr | ctr.att.ail, ctr.att.elv, ctr.att.rud, ctr.eng.thr, ctr.eng.prop, ctr.str.rud |
| RC | cmd.rc.roll, cmd.rc.pitch, cmd.rc.thr, cmd.rc.yaw |
| Usr | est.usr.u1, est.usr.u2, est.usr.u3, est.usr.u4, est.usr.u5, est.usr.u6 |

---

## Adding a New Filter Type (future)

1. Create `FilterMyType.qml` in `Signals/` — a `Fact { flags: Fact.Group }` that exposes `coef`/`coefs` and implements `applyFilter(v)` returning filtered value.
2. Add `FilterMyType { id: fMyType; name: "my_type" … }` inside `MenuFilters.qml`.
3. Add `"my_type"` to the `enumStrings` of `fTypes` in `MenuFilters.qml`.
4. Add a `case "my_type":` branch in `MenuItem.qml`'s `updateValue()`.

---

## Files Summary (post-refactor)

### Added to Signals/
- ChartsView.qml
- PageButton.qml
- MenuSet.qml (replaces FcMenuSet — now manages pages, not sets)
- MenuPage.qml (new — per-page Fact editor)
- MenuItem.qml
- MenuFilters.qml
- FilterRunningAvg.qml
- FilterKalmanSimple.qml
- MenuColor.qml
- ColorChooser.qml
- SignalsMenu.qml (like NumbersMenu — manages sets)

### Deleted from Signals/
- SignalsView.qml
- SignalButton.qml

### Deleted entirely
- src/Plugins/Tools/FilteredCharts/ (whole directory)
