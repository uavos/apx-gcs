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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Common

// Start pages of the checklist: first the operator name (mandatory, the last entered
// one is offered again), then the camera record agreement. The answer goes to
// checklist.cameraDecision(index, name), index < 0 declines.
Item {
    id: page

    property var checklist
    property var theme

    readonly property var cameras: checklist.cameras
    readonly property real fs: theme.fs
    readonly property real sp: theme.sp

    readonly property string operatorName: operatorField.text.trim()
    property bool nameDone: false    // the operator page is answered, the camera one is shown

    // the name entered last time is offered again
    Settings {
        id: settings
        category: "Checklist"
        property string operatorName: ""
    }

    function decide(index)
    {
        settings.operatorName = operatorName
        checklist.cameraDecision(index, operatorName)
    }

    // ---------------------------------------------------------------- page 1: operator
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width, fs*36)
        spacing: sp*4
        visible: !page.nameDone

        MaterialIcon {
            Layout.alignment: Qt.AlignHCenter
            name: "account"
            size: fs*8
            color: theme.textInfo
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Who performs the check?")
            color: theme.textBright
            font: apx.font_narrow(theme.fontTitle, true)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        TextField {
            id: operatorField
            Layout.fillWidth: true
            text: settings.operatorName
            placeholderText: qsTr("Operator full name")
            font: apx.font_narrow(theme.fontStep)
            color: "#fff"
            placeholderTextColor: theme.textMuted
            horizontalAlignment: TextInput.AlignHCenter
            selectByMouse: true
            Material.accent: theme.currentMark
            Material.theme: Material.Dark
            focus: !page.nameDone
            onAccepted: if(page.operatorName !== "") page.nameDone = true
        }
        ChecklistButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Continue")
            kind: "primary"
            enabled: page.operatorName !== ""
            onTriggered: page.nameDone = true
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("The name is recorded in the log of the check.")
            color: theme.textMuted
            font: apx.font_narrow(theme.fontSmall)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }

    // ---------------------------------------------------------------- page 2: camera
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width, fs*36)
        spacing: sp*4
        visible: page.nameDone

        MaterialIcon {
            Layout.alignment: Qt.AlignHCenter
            name: "camera"
            size: fs*8
            color: theme.textInfo
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Record the check with a camera?")
            color: theme.textBright
            font: apx.font_narrow(theme.fontTitle, true)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("A snapshot is taken after every step and saved together with the log of the check to")
                  + "\n" + checklist.recordRoot
            color: theme.text
            font: apx.font_narrow(theme.fontBody)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        ComboBox {
            id: combo
            Layout.fillWidth: true
            model: page.cameras
            enabled: count > 0
            currentIndex: -1
            displayText: currentIndex < 0 ? (count > 0 ? qsTr("Select a camera") : qsTr("No cameras found")) : currentText
            font: apx.font_narrow(theme.fontBody)
            Material.accent: theme.currentMark
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: sp*2
            ChecklistButton {
                text: qsTr("Select and agree")
                kind: "primary"
                enabled: combo.currentIndex >= 0
                onTriggered: page.decide(combo.currentIndex)
            }
            ChecklistButton {
                text: qsTr("No camera")
                kind: "neutral"
                onTriggered: page.decide(-1)
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "<u>" + qsTr("Back to enter name") + "</u>"
            color: backHover.containsMouse ? theme.textBright : theme.textMuted
            font: apx.font_narrow(theme.fontSmall)
            MouseArea {
                id: backHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: page.nameDone = false
            }
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("The answer is recorded in the log of the check.")
            color: theme.textMuted
            font: apx.font_narrow(theme.fontSmall)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }
}
