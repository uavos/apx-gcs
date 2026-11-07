---
page: plugins
---

# EMIP

Engine Monitoring and Indication Panel: RPM and EGT gauges, fuel tanks and annunciators
(LOW FUEL, LOW BAT, SATCOM). Shown in the instruments area next to the system terminal.

Mandala bindings and limits: defaults in `EmipConfig.qml`, overridden by the user file
`<Documents>/UAVOS/emip/emip.json` (same keys). The file is created with defaults on first start.
Without the file the bundled defaults are used; unknown keys are reported in the console.

The plugin also adds the `FP` page (fuel pumps 1-3 and manual mode, `ctr.sw.sw5..sw8`)
to the Controls panel in any layout profile.
