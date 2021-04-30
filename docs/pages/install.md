---
---

# Installation

The [Ground Control](index) (GCS) application binary packages are available for different platforms from the [apx-gcs](https://github.com/uavos/apx-gcs) github repository releases.

To get emails with announcements about new versions, visit [contacts](/contacts) page and subscribe. Although, the installed software will check for updates automatically.

The complete changelog is available here: [CHANGELOG](https://uavos.github.io/apx-gcs/CHANGELOG.html).

## MacOS

* Download the [DMG image](https://github.com/uavos/apx-gcs/releases/latest).
* Load the image and move application to the `Applications` folder.
* Launch the application as usual.
* Install serial port drivers if necessary:
  * [Silicon Labs](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers);
  * [FTDI USB to Serial port converters](https://www.ftdichip.com/Drivers/VCP.htm);

## Linux

* Download the [AppImage](https://github.com/uavos/apx-gcs/releases/latest);
* Make the file executable;
* Launch the application by executing the downloaded file;
* Install the app through menu to have automatic updates;

GCS uses serial port to communicate with the radio modem device. User must be a member of `dialout` group to have rights to access modem device. Execute the following command in terminal to add yourself to the group, then reboot.

```bash
sudo usermod -aG dialout $USER
sudo apt remove modemmanager -y
```

In order to support video streaming, install GStreamer:

```bash
sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav -y
```

More information about AppImage standard can be found here: [appimage.org](https://appimage.org)

## Android

The app can be installed through [Play Market](https://play.google.com/store/apps/details?id=com.uavos.qgc). Although, it requires PC Ground Control app to communicate with UAV and is used mainly for maintenance purposes.

## Deprecated Versions

### APX Ground Control v10.x

Visit the repository [apx-releases](https://github.com/uavos/apx-releases/releases/latest) for downloads. The GCS v10.x is compatible with APX Autopilot firmware of version v10.x, v9.x and below.

### Linux v9.x

Add UAVOS debian repository to your system (execute the following commands in console):

```bash
echo "deb http://apt.uavos.com/ all main experimental"| sudo tee /etc/apt/sources.list.d/uavos.list; gpg --keyserver keyserver.ubuntu.com --recv B5517CF1; gpg --export --armor B5517CF1 | sudo apt-key add -; sudo apt-get update
```

Repository contains two components:

* **main** - rare updates (once in 3-6 months), more bugs, but ensures it was tested on several UAVs for stable operation.
* **experimental** - most recent bugfixes and features, builds for most recent linux versions. Updates almost every week, tested on at least one UAV.

Install application and tools:

```bash
sudo apt-get install uavos
```

Run the application through KDE's menu or `gcu` executable.
