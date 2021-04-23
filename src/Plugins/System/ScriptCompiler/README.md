---
---

# Script Compiler

[WASM](https://webassembly.org) Scripting Engine is supported by onboard MCUs of the APX Autopilot. The script, written in `C++` is pre-compiled by this plugin to be uploaded to a node.

The plugin automatically downloads the compiler binaries and stores it in `Documents/UAVOS/Scripts/compiler`.

The GCS has a built-in script editor, although, the `.vscode` configuration is provided to ease script development in `Documents/UAVOS/Scripts` folder.

To use [Visual Studio Code](https://code.visualstudio.com/) as IDE, follow these steps:

* open GCS built-in script editor for a node parameter;
* save script to a file in `Documents/UAVOS/Scripts` folder;
* open saved file in `VScode`, edit or compile;
* GCS will watch the saved script file for changes and update the node parameter automatically;

Script examples are provided in `Documents/UAVOS/Scripts/examples` folder for reference.
