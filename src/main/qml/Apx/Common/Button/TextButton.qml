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

ButtonBase {
    id: control

    property bool showText: true

    property color textColor: Material.primaryTextColor
    property color disabledTextColor: Material.hintTextColor

    property color currentTextColor: enabled?textColor:disabledTextColor

    property real textScale: 0.6
    readonly property int textSize: Math.max(7, control.height * textScale - 2)

    font.pixelSize: textSize

    contentComponent: Component {
        id: _textC
        Text {
            visible: showText && text
            font: control.font
            text: control.text
            color: control.currentTextColor

            textFormat: Text.PlainText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
    property Component textC: _textC
}
