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

Rectangle {
    id: rect
    z: 9999

    property var item

    anchors.fill: item?null:parent
    color: "transparent"
    border.width: 2
    border.color: "#fff"
    opacity: 0.3

    Component.onCompleted: {
        console.warn(rect, item)
        if(!item)return

        x = Qt.binding(function(){return itemRect().x})
        y = Qt.binding(function(){return itemRect().y})
        width = Qt.binding(function(){return item.width})
        height = Qt.binding(function(){return item.height})
    }
    function itemRect()
    {
        var p=item.parent.mapToGlobal(item.x, item.y)
        return parent.mapFromGlobal(p.x,p.y)
    }
}
