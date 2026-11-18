---
toc: true
---

# Plugins

All user plugins should be installed in `Documents/UAVOS/Plugins` folder.

## Binary plugins

The application loads plugins provided with the app bundle and any other file from user plugins folder. The supported file types are: `.so`, `dylib`, `.bundle`, `gcs`. If a user plugin has the same name as any app bundle plugin - it overrides the original.

## QML plugins

Any `.qml` file existing in the user plugins root folder will be loaded and created in the QML context of the application.

## Main window layout customization

The main UI layout is defined in `GroundControl.qml`. When this file exists in `Documents/UAVOS/Plugins/main` folder - it overrides the main window layout and UI.

The layout file may hide interface elements and set the default sizes of the top panels:

```qml
import Apx.Application

GroundControl {
    hiddenItems: ["nodes", "controls", "choke"]

    instrumentsScale: 0.25              // panels height, fraction of the window width
    panelSizes: ({ "pfd": 0.4 })        // panels widths: status, pfd, numbers, commands
    panelOrder: ["instruments", "pfd", "numbers", "commands", "status"] // left to right
    layoutLocked: false                 // true to forbid resizing and moving by the user
}
```

The user resizes the panels by dragging the separators after pressing the `Adjust panels size` button of the task bar (double click on a separator restores the default size) and moves a panel to another place by dragging the icon in its center. The result is saved to `Documents/UAVOS/Preferences/layout.json` and takes precedence over the layout file values, unless `layoutLocked` is set.

## Examples

See the source code of the GCS application for plugins [examples](https://github.com/uavos/apx-gcs/tree/main/src/Plugins).
