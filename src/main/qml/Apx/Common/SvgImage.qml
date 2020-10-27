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
import QtQuick 2.2
import QtGraphicalEffects 1.0

Item {
    property alias source: image.source
    property alias sourceSize: image.sourceSize
    property alias color: imageColor.color

    property Component effect

    //smooth: ui.antialiasing
    width: image.width
    height: image.height
    Image {
        id: image
        fillMode: Image.PreserveAspectFit
        //smooth: ui.antialiasing
    }
    ColorOverlay {
        id: imageColor
        anchors.fill: image
        source: image
        color: "#FFFFFF"
        cached: true
        layer.enabled: effect
        //layer.smooth: ui.antialiasing
        layer.effect: effect
    }
}
