---
---

# Onboard scripting

[WASM](https://webassembly.org) scripting Engine is supported by [onboard MCUs](/hw) of the APX Autopilot. The script, written in `C++` is pre-compiled by this plugin to be uploaded to a node.

The plugin automatically downloads the required compiler binaries and stores it in `Documents/UAVOS/Scripts/compiler` folder. The compiler is based on [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) distribution.

## Script IDE

The GCS has a built-in script editor, which has the basic functionality to edit, compile and save/load files.

### Visual Studio Code IDE

The [`.vscode`](https://github.com/uavos/apx-gcs/tree/main/resources/scripts/.vscode) configuration is provided to ease script development in `Documents/UAVOS/Scripts` folder. The `.vscode` files are copied by GCS from its resources. The configuration provides support for script compilation, linter, formatting, tasks, etc.

To use [Visual Studio Code](https://code.visualstudio.com/) as IDE, follow these steps:

* open GCS built-in script editor for a node parameter;
* save script to a file in `Documents/UAVOS/Scripts` folder;
* open the workspace `Documents/UAVOS/Scripts` in `VScode`, edit the saved file as desired;
* compile the file by using the provided task (for debug/linter purposes);
* GCS will watch the saved script file for changes and update the node parameter automatically;

>When GCS script editor saves a script to a file or loads from a file - such file is watched for modifications by the GCS and the node configuration parameter is updated accordingly upon changes of the file's content are detected.
>
>The node parameter, which have the `script` as its content will be compiled by the GCS on it's value changes. You may need to `upload` the vehicle parameters to bring the compiled script to run onboard;

## LLVM `sysroot`

Every script should have the following statement to interact with the APX node resources:

```cpp
#include <apx.h>
```

This will include the `apx.h` file from `sysroot/include` folder, which declares all APX node [interfaces](/fw). See the example called `ApxTest.cpp` for use cases.

The provided `clang` WASM compiler is invoked by the GCS with the option `--sysroot=<path to sysroot>`, according to [this configuration](https://github.com/uavos/apx-gcs/blob/main/resources/scripts/.vscode/tasks.json).

More information can be found here:

* Local `Documents/UAVOS/Scripts/sysroot` folder - this is the `sysroot` of the scripts compiled by the GCS;
* [sysroot](https://github.com/uavos/apx-gcs/tree/main/resources/scripts/sysroot) - the source code of the sysroot, which is copied to user folder;
* [defined-symbols.txt](https://github.com/uavos/apx-gcs/blob/main/resources/scripts/sysroot/share/defined-symbols.txt) - list of defined/linkable symbols, i.e. a node provides native implementation of these symbols;

## Examples

Script examples are provided in `Documents/UAVOS/Scripts/examples` folder for reference, and are copied by the GCS from its resources.

>Check [UAVOS GitHub organization](https://github.com/uavos) for public repositories.
