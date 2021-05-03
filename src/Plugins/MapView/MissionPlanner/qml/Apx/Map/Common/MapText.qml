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
import QtQuick 2.5;

Rectangle {
    id: mapText

    property alias text: textItem.text
    property alias font: textItem.font
    property alias horizontalAlignment: textItem.horizontalAlignment
    property alias verticalAlignment: textItem.verticalAlignment
    property alias textColor: textItem.color

    property bool square: false

    property real margins: 0
    property real rightMargin: 0
    property real minWidth: 0
    property real minHeight: font.pointSize+mapText.margins*2+1

    Behavior on implicitWidth { enabled: ui.smooth; NumberAnimation {duration: 100; } }
    Behavior on implicitHeight { enabled: ui.smooth; NumberAnimation {duration: 100; } }

    border.width: 0
    color: "gray"
    //smooth: ui.antialiasing
    implicitWidth: (square?textItem.width:textItem.contentWidth)+mapText.margins*2+1+rightMargin
    implicitHeight: textItem.height+mapText.margins*2+1
    radius: height/10
    //clip: true
    Text {
        id: textItem
        x: mapText.margins
        y: mapText.margins
        color: "white"
        font: apx.font_narrow(map.fontSize)
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        width: Math.max(minWidth-mapText.margins*2-1-rightMargin,square?Math.max(contentWidth,contentHeight):implicitWidth)
        height: Math.max(minHeight-mapText.margins*2-1,contentHeight)
    }
}
