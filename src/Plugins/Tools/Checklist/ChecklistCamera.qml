/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtCore
import QtMultimedia

// Snapshot camera of the check: the operator picks one of the video inputs on the consent
// page (selected < 0: no camera), capture(path) saves a JPEG there.
QtObject {
    id: control

    readonly property var devices: mediaDevices.videoInputs
    property var names: []          // descriptions of the devices, for the consent page
    property int selected: -1
    readonly property bool enabled: selected >= 0 && selected < devices.length
    readonly property string name: enabled ? String(devices[selected].description) : ""

    // OS camera access: the media backend only checks it, the request is on the app
    property CameraPermission permission: CameraPermission {
        onStatusChanged: if(status === Qt.PermissionStatus.Denied) control.failed(qsTr("camera access denied"))
    }
    readonly property bool granted: permission.status === Qt.PermissionStatus.Granted
    onEnabledChanged: if(enabled && !granted) permission.request()

    signal saved(string path)
    signal failed(string message)

    property MediaDevices mediaDevices: MediaDevices {
        onVideoInputsChanged: control.update()
    }
    Component.onCompleted: update()
    function update()
    {
        var list = []
        for (var i = 0; i < devices.length; ++i) list.push(String(devices[i].description))
        names = list
    }

    property CaptureSession session: CaptureSession {
        camera: Camera {
            id: camera
            active: control.enabled && control.granted
            cameraDevice: control.enabled ? control.devices[control.selected] : control.mediaDevices.defaultVideoInput
            onErrorOccurred: (error, message) => control.failed(message)
        }
        imageCapture: ImageCapture {
            id: capture
            onImageSaved: (id, path) => control.saved(path)
            onErrorOccurred: (id, error, message) => control.failed(message)
        }
    }
    function capture(path)
    {
        if(!enabled || !camera.active) return false
        if(!capture.readyForCapture){
            failed(qsTr("camera is not ready"))
            return false
        }
        capture.captureToFile(path)
        return true
    }
}
