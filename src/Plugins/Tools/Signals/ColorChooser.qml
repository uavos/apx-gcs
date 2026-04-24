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
import QtQuick.Controls.Material

Rectangle {
    id: editor

    readonly property string colorText: fact && fact.colorValue !== undefined
                                        ? String(fact.colorValue).trim().toUpperCase()
                                        : (fact && fact.itemOwner && fact.itemOwner.colorValue !== undefined
                                           ? String(fact.itemOwner.colorValue).trim().toUpperCase()
                                           : "")
    readonly property bool selected: fact && fact.itemOwner
                                     && fact.itemOwner.colorValue === colorText

    implicitHeight: factButton.height * 0.6
    implicitWidth: factButton.height * 1.8
    radius: height / 5
    border.width: selected ? 2 : 1
    border.color: selected ? "white" : "#66000000"
    color: colorText !== "" ? colorText : "transparent"

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        visible: colorText === ""
        radius: parent.radius - 1
        color: "transparent"
        border.width: 1
        border.color: Material.hintTextColor

        Text {
            anchors.centerIn: parent
            text: qsTr("A")
            font: apx.font_narrow(Math.max(10, parent.height * 0.55))
            color: Material.hintTextColor
        }
    }
}
