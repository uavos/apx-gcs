pragma ComponentBehavior: Bound

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

import Apx.Common

ValueButton {
    id: control

    property bool selected: false
    property bool pinned: false
    property bool pageWarning: false

    hoverEnabled: true
    checkable: true
    checked: selected
    active: selected

    showValue: false
    alerts: false
    warning: pageWarning
    normalColor: pinned ? "#4a4a22" : "#202020"
    textBold: selected
    textScale: 0.62
    minimumWidth: Math.max(28, height * 1.15)
    maximumWidth: Math.max(minimumWidth, height * 3.0)
    toolTipItem.delay: 500

    onTriggered: checked = true

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: "transparent"
        border.width: control.pinned ? 1 : 0
        border.color: control.pageWarning
                      ? Material.color(Material.Orange)
                      : Material.color(Material.BlueGrey)
    }

    MaterialIcon {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 2
        visible: control.pinned
        name: "pin"
        size: Math.max(8, control.height * 0.34)
        color: Material.color(Material.LightBlue)
    }
}
