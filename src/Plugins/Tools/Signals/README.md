---
page: plugins
---

# Signals

QML widget to show live charts of UAV telemetry values for tuning and diagnostics.

The refactored Signals plugin now supports:

- Multiple saved sets in `signals.json`
- Named pages inside each set
- Pinned pages rendered as stacked charts
- Per-page speed persistence with values `0.2x`, `0.5x`, `1x`, `2x`, `4x`
- Per-item colors, warning/alarm expressions, actions, and save-to-`sns.scr.*`
- Ordered, draggable filter chains with `running_avg` and `kalman_smp`
