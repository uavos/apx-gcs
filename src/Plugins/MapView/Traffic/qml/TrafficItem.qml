/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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
import QtLocation
import QtPositioning

import Apx.Common

// ADS-B aircraft: icon rotated by track and a short data label
MapQuickItem {
    id: item

    readonly property color levelColor: model.level===2 ? "#FF4C00"
                                      : model.level===1 ? "#FFC800"
                                      : "#00FFFF"

    coordinate: model.coordinate
    zoomLevel: 0
    anchorPoint.x: icon.width/2
    anchorPoint.y: icon.height/2

    sourceItem: Item {
        width: icon.width
        height: icon.height

        MaterialIcon {
            id: icon
            name: "airplane"
            size: Style.buttonSize
            color: levelColor
            rotation: model.track
            style: Text.Outline
            styleColor: "#000"
        }
        Text {
            anchors.left: icon.right
            anchors.verticalCenter: icon.verticalCenter
            anchors.leftMargin: 2
            text: model.label
            color: levelColor
            font: apx.font_narrow(Style.fontSize*0.8)
            style: Text.Outline
            styleColor: "#000"
        }
    }
}
