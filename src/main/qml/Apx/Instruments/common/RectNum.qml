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
import "."

Rectangle {
    id: _control
    property string value
    property string toolTip
    color: enabled?"#30000000":"transparent"
    //color: "#225d9d"
    border.width: 1
    border.color: "white"

    property alias textColor: _text.color

    property bool enabled: true

    Text {
        id: _text
        anchors.fill: parent
        anchors.margins: 1
        anchors.topMargin: 2
        anchors.rightMargin: 1
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text:  value
        font: apx.font_narrow(height)
        color: _control.enabled?"magenta":"gray"
    }
    ToolTipArea {
        text: _control.toolTip
    }
}

