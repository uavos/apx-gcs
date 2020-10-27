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
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import ".."

TextButton {
    id: control

    property string iconName
    property bool showIcon: true

    property color iconColor: Material.iconColor
    property color disabledIconColor: Material.iconDisabledColor

    property real iconScale: 0.9
    property int iconSize: Math.max(7, control.height * iconScale - 2)

    property color currentIconColor: enabled?iconColor:disabledIconColor

    contentComponent: Component {
        id: _iconC
        MaterialIcon {
            visible: showIcon && iconName
            size: iconSize
            color: currentIconColor
            name: control.iconName

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    property Component iconC: _iconC
}
