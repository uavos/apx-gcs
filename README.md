# APX Autopilot by UAVOS - Ground Control Software

![APX GCS CI](https://github.com/uavos/apx-gcs/workflows/APX%20GCS%20CI/badge.svg)

[![GSC video](http://img.youtube.com/vi/CSPNkGZuP8M/0.jpg)](https://youtu.be/CSPNkGZuP8M)

APX Ground Control Software (`GCS`): open-source project, leaded by [UAVOS Company](http://uavos.com).

This repository contains a cross-platform application, which is part of [APX Autopilot project](http://docs.uavos.com).

The purpose of this application is to manage command & control, mission planing, and safety requirements of [Unmanned Vehicle](https://en.wikipedia.org/wiki/UAV) (UAV).

The source code, published in this repository, **is not based** on any known open-source projects related to UAV industry.

## Key features

- [Open Source](https://github.com/uavos/apx-gcs/blob/main/LICENSE);
- Based on [QT framework](https://www.qt.io) and [QML](https://en.wikipedia.org/wiki/QML);
- Multi-platform;
- Customizable [frontend](https://github.com/uavos/apx-gcs/blob/main/src/main/qml/Apx/Application/GroundControl.qml);
- C++ defined JS Tree at the [backend](https://github.com/uavos/apx-gcs/tree/main/src/lib/ApxCore/Fact);
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
- `Sounds`: reads out autopilot warnings and messages with defined voice and [TTS](https://en.wikipedia.org/wiki/Speech_synthesis) engines, availebale to the system, and emits emergency alarm sounds;
- `TelemetryChart`: [QWT](https://qwt.sourceforge.io)-lib based widget to visualize telemetry database;
- `Terminal`: UAV commands termial, [JavaScript](https://wiki.qt.io/JavaScript)/QML based;
- `TreeFacts`: general back-end qobjects data structure tree-view, used for debugging;
- `TreeJS`: JavaScript context tree view, used for debugging and reference;
- `VehicleConfiguration`: UAV parameters tree view and extended QTreeView-based editor;
- `VideoStreaming`: [GStreamer](https://gstreamer.freedesktop.org) based video streaming low-latency plugin with UAV gimbal controls and video stream recording;


## Build instructions

### Clone the repository

```bash
git clone --recurse-submodules git@github.com:uavos/apx-gcs.git
```

GCS project uses [APX Shared Library](https://github.com/uavos/apx-lib) submodule.

### Required libraries

- [GStreamer](https://gstreamer.freedesktop.org) - used for video streaming by some plugins;
- [SDL2](https://www.libsdl.org) - used for joystick interface by some plugins;
- [Sparkle](https://sparkle-project.org/) - required for mac auto updates;
- [AppImageUpdate](https://github.com/AppImage/AppImageUpdate) - required for linux build, see installation in [Dockerfile](https://github.com/uavos/apx-gcs/blob/main/Dockerfile);

### CMAKE build

The project uses `cmake` build system. Some tools require `python3` scripts to manage source files generation via [`jinja`](https://jinja.palletsprojects.com) and assemble deploy [packages](https://github.com/uavos/apx-gcs/blob/main/cmake/apx_gcs_deploy.cmake) for the specified platform.

For the required tools, take a look at [Dockerfile](https://github.com/uavos/apx-gcs/blob/main/Dockerfile) and [GitHub CI](https://github.com/uavos/apx-gcs/blob/main/.github/workflows/apx-gcs-release.yml).


#### Building binaries

After cloning the repos and installing required tools, use the following commands to build the `build/out/bin/gcs` runtime:

```bash
cd apx-gcs
cmake -H. -Bbuild -G Ninja
cmake --build build
```

Launch the `GCS` app:

```bash
./build/out/bin/gcs
```

#### Building application bundle with CMake

To create the GCS application bundle - use [`bundle`](https://github.com/uavos/apx-gcs/blob/main/cmake/apx_gcs_deploy.cmake) target:

```bash
cmake --build build --target bundle
```

Depending on the host platform, this will build either `.app` macos application or the [`AppImage`](https://appimage.org) linux bundle.

>Take a look at some additional targets in [Makefile](https://github.com/uavos/apx-gcs/blob/main/Makefile). Although, the makefile is used mainly to create releases.

### Docker build

The [`Dockerfile`](https://github.com/uavos/apx-gcs/blob/main/Dockerfile) is included in this repository and may be used as a reference for the tools required to build the application for the `linux` platform.

See the [`Docker.mk`](https://github.com/uavos/apx-gcs/blob/main/Docker.mk) file for details.

```bash
cd apx-gcs
docker pull uavos/apx-gcs-linux:latest
make docker-run
cmake -H. -Bbuild -G Ninja
cmake --build build
```

### Output directories

- `build/out` contains runtime binaries;
- `build/deploy` contains app bundle;

## Runtime requirements

`GCS` uses serial ports to communicate with the radio modem device. This might require some drivers to be installed on the host platform.

### MacOS

The modern APX nodes, starting form version `11` - support [CDC](https://en.wikipedia.org/wiki/USB_communications_device_class). There is no need to install any drivers on the host side. Although, you may find the following links useful:

- [Silicon Labs](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers) USB Serial Ports;
- [FTDI](https://www.ftdichip.com/Drivers/VCP.htm) USB Serial Ports;

MacOS app bundle comes with all frameworks built-in and does not require additional actions to perform.

### Linux

User should be a member of `dialout` group to have rights to access the modem device. Execute the following command to add yourself to the group, then reboot.

```bash
sudo usermod -aG dialout $USER
sudo apt remove modemmanager -y
```

AppImage does not include the `GStreamer` library, and in order to support video streaming - install `GStreamer` runtime system-wide:

```bash
sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav -y
```

More information about AppImage standard can be found here: [appimage.org](https://appimage.org)

## Links

- [Changelog](https://uavos.github.io/apx-gcs/CHANGELOG.html);
- [uavos/apx-ap](https://github.com/uavos/apx-ap) - APX Autopilot Firmware packages;
- [UAVOS Inc. company web site](http://uavos.com) with products and more;
- [APX Autopilot documentation](http://docs.uavos.com)
- [UAVOS Inc. GitHub Organization](https://github.com/uavos)

#   

>&copy; [Aliaksei Stratsilatau](https://github.com/uavinda)
