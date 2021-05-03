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
import QtQuick 2.6
import QtQuick.Controls 2.1

import Apx.Controls 1.0

import "../common"

Item{

    implicitWidth: 800
    implicitHeight: 400


    opacity: ui.effects?(loaded?1:0.3):1
    property bool loaded: comm.status === Loader.Ready
                          && pfd.status === Loader.Ready
                          && nav.status === Loader.Ready
    Behavior on opacity { enabled: ui.smooth; PropertyAnimation {duration: 500} }

    Loader {
        id: comm
        asynchronous: ui.smooth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: apx.limit(parent.height*0.05,10,width/32)
        source: "../comm/Comm.qml"
    }

    Loader {
        id: pfd
        asynchronous: ui.smooth
        anchors.fill: parent
        anchors.topMargin: comm.height
        anchors.bottomMargin: apx.limit(comm.height,20,comm.height)
        source: "../pfd/Pfd.qml"
    }

    Rectangle {
        id: btm_sep
        border.width: 1
        border.color: "#770"
        height: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: pfd.bottom
    }

    Loader {
        id: nav
        asynchronous: ui.smooth
        anchors.top: btm_sep.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        source: "../nav/Nav.qml"
    }

}
