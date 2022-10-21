---
---

# Installation

The [Ground Control](index) (GCS) application binary packages are available for different platforms from the [apx-gcs](https://github.com/uavos/apx-gcs) github repository releases.

To get emails with announcements about new versions, visit [contacts](/contacts) page and subscribe. Although, the installed software will check for updates automatically.

The complete changelog is available here: [CHANGELOG](https://uavos.github.io/apx-gcs/CHANGELOG.html).

## MacOS

* Download the [DMG image](https://github.com/uavos/apx-gcs/releases/latest);
* Open the image and move application to the `Applications` folder;
* Launch the application as usual;

## Linux

* Download the [AppImage](https://github.com/uavos/apx-gcs/releases/latest) of the GCS application;
* Make the file executable;
* Launch the application by executing the downloaded file;
* Install the app through menu to have automatic updates;

GCS uses serial port to communicate with the radio modem device. User must be a member of `dialout` group to have rights to access modem device. Execute the following command in terminal to add yourself to the group, then reboot.

```bash
sudo usermod -aG dialout $USER
sudo apt remove modemmanager -y
```

In order to support video streaming, install *GStreamer*:

```bash
sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav -y
```

More information about *AppImage* can be found here: [appimage.org](https://appimage.org)

## Android

The app can be installed through [Play Market](https://play.google.com/store/apps/details?id=com.uavos.qgc). Although, it requires PC Ground Control app to communicate with UAV, and is used mainly for maintenance purposes. This app now supports v9 firmware only.

## Deprecated Versions

### APX Ground Control v10.x

GCS v10 software is available [here](https://drive.google.com/drive/folders/1GEVWtF4XueCrVSYZj6rxiJNdCmD3dZPv). The GCS v10.x is compatible with firmware of version v10.x, v9.x and below.

### Linux Packages v9.x

Debian packages are available [here](https://drive.google.com/drive/folders/1V_2b9UXeTBPWj0aAzdIPjoFLYsnMdqbV).

```bash
sudo apt install <path to downloaded files>/*.deb
```

Run the application through KDE's menu or `gcu` executable.
