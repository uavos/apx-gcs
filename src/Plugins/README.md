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

## Examples

See the source code of the GCS application for plugins [examples](https://github.com/uavos/apx-gcs/tree/main/src/Plugins).
