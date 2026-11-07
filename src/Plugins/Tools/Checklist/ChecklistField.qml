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

// labeled input field of the checklist form
RowLayout {
    id: control

    property string label
    property alias value: field.text
    signal accepted()

    spacing: ChecklistStyle.sp

    Text {
        text: control.label
        color: ChecklistStyle.text
        font: apx.font_narrow(ChecklistStyle.fontField)
    }
    TextField {
        id: field
        Layout.fillWidth: true
        Layout.preferredWidth: ChecklistStyle.fieldWidth
        font: apx.font_narrow(ChecklistStyle.fontField)
        color: ChecklistStyle.textBright
        selectByMouse: true
        Material.accent: ChecklistStyle.currentMark
        onAccepted: control.accepted()
    }
}
