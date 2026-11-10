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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Common

// one field of the checklist form: text input, drop-down or checkbox across the full width
ColumnLayout {
    id: control

    property string label
    property string type: "input"    // input | select | checkbox
    property var options: []         // select items
    property var value: ""           // default from the step script
    signal accepted()

    // current state reported back to the script on submit (string; bool for checkbox)
    readonly property var result: type === "checkbox" ? check.checked
                                : type === "select" ? combo.currentText
                                : field.text

    readonly property real fs: Style.fontSize*1.4

    spacing: 0

    Text {
        Layout.fillWidth: true
        visible: control.type !== "checkbox"
        text: control.label
        color: "#ddd"
        font: apx.font_narrow(control.fs)
        wrapMode: Text.Wrap
    }
    TextField {
        id: field
        visible: control.type === "input"
        Layout.fillWidth: true
        text: control.type === "input" ? String(control.value) : ""
        font: apx.font_narrow(control.fs)
        color: "#fff"
        selectByMouse: true
        Material.accent: "#4a8df8"
        onAccepted: control.accepted()
    }
    ComboBox {
        id: combo
        visible: control.type === "select"
        Layout.fillWidth: true
        model: control.options
        currentIndex: Math.max(0, control.options.indexOf(String(control.value)))
        font: apx.font_narrow(control.fs)
        Material.accent: "#4a8df8"
        Material.theme: Material.Dark
    }
    CheckBox {
        id: check
        visible: control.type === "checkbox"
        Layout.fillWidth: true
        text: control.label
        checked: control.value === true || control.value === "true"
        font: apx.font_narrow(control.fs)
        Material.accent: "#4a8df8"
        Material.theme: Material.Dark
        Material.foreground: "#ddd"
    }
}
