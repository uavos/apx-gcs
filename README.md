# APX Autopilot by UAVOS - Ground Control Software

GCS - APX Ground Control Software.

This application is part of [APX Autopilot project](http://docs.uavos.com).

## Key features

 - [Open Source](https://github.com/uavos/apx-gcs/blob/main/LICENSE);
 - Multi-platform;
 - Based on [QT](https://www.qt.io) framework;
 - QML [frontend](https://github.com/uavos/apx-gcs/blob/main/src/main/qml/Apx/Application/GroundControl.qml);
 - Tree of qobjects at the [back-end](https://github.com/uavos/apx-gcs/tree/main/src/lib/ApxCore/Fact);
 - Application defined by [plugins](https://github.com/uavos/apx-gcs/tree/main/src/Plugins);
 - Protocols [abstraction](https://github.com/uavos/apx-gcs/tree/main/src/lib/ApxData/Protocols);

## Included Plugins
 - `AppUpdate`: automatic application update support for macos via [Sparkle](https://sparkle-project.org/) for mac or [AppImage](https://appimage.org) for linux;
 - `CompassCalibration`: helper widget for hard-iron compass calibration;
 - `DatalinkInspector`: QML widget for inspecting of datalink packets;
 - `FirmwareLoader`: allows to update firmware of [APX nodes](http://docs.uavos.com/hw/index.html) or upload to stm32 [bootloader](https://www.st.com/resource/en/application_note/cd00264342-usart-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf);
 - `Joystick`: makes [SDL2](https://www.libsdl.org) contols available to the UAV control logic;
 - `Location`: geo map tiles downloader and offline cache, optimized for UAV applications;
 - `MandalaTree`: tree view window of UAV state;
 - `MissionPlanner`: [QML map](https://doc.qt.io/qt-5/qml-qtlocation-map.html) with mission editor;
 - `ScriptCompiler`: [WASM](https://webassembly.org) Scripting engine support for onboard MCUs;
 - `SerialPortConsole`: tool to debug and trace serial data from onboard serial ports ([VCP](http://docs.uavos.com/fw/conf/serial.html));
 - `ServoConfig`: tool to configure some specific servo drives;
 - `Shortcuts`: manages keyboard shortcuts and assigned commands to the UAV;
 - `Signals`: QML widget to show live chart of defined UAV physical values for easy tuning;
 - `Simulator`: launches [X-Plane](https://www.x-plane.com) flight simulator with provided specific plugins to communicate to the GCS for SIL/HIL simulation;
 - `Sites`: map view plugin to add named areas to missions and map areas;
 - `Sounds`: reads out autopilot warnings and messages with defined voice and [TTS](https://en.wikipedia.org/wiki/Speech_synthesis) engines and emits emergency alarms;
 - `TelemetryChart`: [QWT](https://qwt.sourceforge.io) based widget to review telemetry database;
 - `Terminal`: UAV commands termial, [JavaScript](https://wiki.qt.io/JavaScript) based;
 - `TreeFacts`: general back-end qobjects data structure tree-view, used for debugging;
 - `TreeJS`: JavaScript context tree view, used for debugging and reference;
 - `VehicleConfiguration`: UAV parameters tree view and extended editor;
 - `VideoStreaming`: [GStreamer](https://gstreamer.freedesktop.org) based video streaming low-latency plugin with UAV gimbal controls and video stream recording;


## Build instructions

### Clone the repository

```
git clone --recurse-submodules git@github.com:uavos/apx-gcs.git gcs
```

GCS project uses [APX Shared Library](https://github.com/uavos/apx-lib) submodule.

### Cmake build

The project uses `cmake` build system. Some tools require `python3` scripts (see [Makefile](https://github.com/uavos/apx-gcs/blob/main/Makefile) for required tools) to manage source files generation via [`jinja`](https://jinja.palletsprojects.com) and assemble deploy [packages](https://github.com/uavos/apx-gcs/blob/main/cmake/apx_gcs_deploy.cmake) for the specified platform.

It is required for GCS compilation to have [`apx-lib`](https://github.com/uavos/apx-lib) shared library source available in the parent directory of the project.

For required tools, see [Makefile](https://github.com/uavos/apx-gcs/blob/main/Makefile) `install-tools-XXX` targets.

#### Required libraries
 - [GStreamer](https://gstreamer.freedesktop.org) - used for video streaming by some plugins;
 - [SDL2](https://www.libsdl.org) - used for joystick interface by some plugins;
 - [Sparkle](https://sparkle-project.org/) - required for mac auto updates;

After cloning the repos, use the following commands to build the `build/out/bin/gcs` runtime:
```
cd gcs
cmake -H. -Bbuild -G Ninja
cmake --build build
```

### Building application bundle

To create the GCS application bundle with all libs installed - use [`bundle`](https://github.com/uavos/apx-gcs/blob/main/cmake/apx_gcs_deploy.cmake) target:

```
cmake --build build --target bundle
```

This will build either `.app` macos application of [`AppImage`](https://appimage.org) linux bundle, depending on the host platform;

>Take a look at some additional targets in [Makefile](https://github.com/uavos/apx-gcs/blob/main/Makefile).

### Output directories

 - `build/out` contains runtime binaries;
 - `build/deploy` contains app bundle;
