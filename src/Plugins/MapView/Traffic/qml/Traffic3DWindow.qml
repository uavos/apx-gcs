/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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
import QtQuick.Window

// Traffic 3D window (see Traffic3DView.qml), loaded by Fact::loadQml as a root
// object: a Window parented to an Item outside of any window would never be shown
Window {
    id: window

    // set by the Traffic plugin on creation: the window toggle fact and the plugin itself
    property var fact
    property var traffic

    title: qsTr("Traffic 3D")
    width: 1280
    height: 800
    minimumWidth: 800
    minimumHeight: 500
    color: "#0b1016"
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint | Qt.WindowMinMaxButtonsHint

    // visibility follows the fact; closing only resets the fact to keep the binding
    visible: fact ? fact.value : false
    onClosing: (close) => {
        close.accepted = false
        fact.value = false
    }

    Traffic3DView {
        anchors.fill: parent
        traffic: window.traffic
        running: window.visible
    }
}
