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
import QtQuick.Controls 2.15

Item {
    id: control
    property Component iconC
    property Component textC
    property Component valueC

    implicitWidth: _titleRow.implicitWidth + _valueItem.implicitWidth + 2
    implicitHeight: 24

    Row {
        id: _titleRow
        spacing: 0
        anchors.fill: parent
        //anchors.rightMargin: Math.min(_valueItem.implicitWidth, _valueItem.width)
        //clip: true

        // icon
        Loader {
            id: _icon
            height: parent.height
            width: item?height:0
            sourceComponent: iconC
        }

        // title
        Loader {
            id: _title
            height: parent.height
            sourceComponent: textC
        }
    }
    Loader {
        id: _valueItem
        anchors.fill: parent
        anchors.leftMargin: (_icon.item?_icon.width:0) + (_title.item?_title.width:0)
        sourceComponent: valueC
    }
}
