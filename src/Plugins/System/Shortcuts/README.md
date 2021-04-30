---
page: plugins
---

# Keyboard Shortcuts

Plugin manages keyboard shortcuts configuration. Any configured keyboard shortcut is triggering JS script evaluation, which is assigned to the shortcut.

F.ex. `B` button press will call `btn_BRAKE()` JS function defined in app bundle system `gcs.js` file resource.

The shortcuts configuration is split in two parts:

* System shortcuts - defined in app bundle, but can be disabled individually or overridden by user shortcuts;
* User shortcuts - completely configured by user;

Additional JS functions can be defined in `Documents/UAVOS/Scripts/gcs.js` user file.
