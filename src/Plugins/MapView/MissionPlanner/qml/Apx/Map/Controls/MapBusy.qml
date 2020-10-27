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
import QtQuick          2.3
import QtQuick.Controls 2.3
import QtLocation       5.3
import QtQml 2.12

Item {
    id: control

    property var fact: apx.tools?apx.tools.location:null
    property string text: fact?fact.text:""
    property int progress: fact?fact.progress:-1

    implicitWidth: 100
    implicitHeight: 10

    visible: progress>=0

    ProgressBar {
        anchors.fill: parent
        value: progress
        indeterminate: true
        opacity: ui.effects?0.9:1
        background.height: height
        contentItem.implicitHeight: parent.height
    }

    //running: visible
    Label {
        anchors.fill: parent
        color: "#60FFFFFF"
        text: control.text
        font.bold: true
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        font.pixelSize: Math.max(8,height)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            fact.abort();
        }
    }
}
