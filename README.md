# APX Autopilot by UAVOS - Ground Control Software

![APX GCS CI](https://github.com/uavos/apx-gcs/workflows/APX%20GCS%20CI/badge.svg)

[![GSC video](http://img.youtube.com/vi/CSPNkGZuP8M/0.jpg)](https://youtu.be/CSPNkGZuP8M)

APX Ground Control Software (`GCS`) is open-source project, leaded by [UAVOS Company](http://uavos.com).

This repository contains a cross-platform application, which is part of [APX Autopilot project](http://docs.uavos.com).

The purpose of this application is to manage command & control, mission planing, and safety requirements of [Unmanned Vehicle](https://en.wikipedia.org/wiki/UAV) (UAV).

The source code, published in this repository, **is not based** on any known open-source projects related to UAV industry.

More information about GCS application and its internals can be found here: [docs.uavos.com](http://docs.uavos.com)

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
cmake --build build --target deploy_package
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
