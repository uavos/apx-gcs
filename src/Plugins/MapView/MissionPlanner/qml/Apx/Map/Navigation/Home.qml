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
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Map.Common 1.0

MapQuickItem {  //to be used inside MapComponent only

    //Fact bindings
    property real home_lat: mandala.est.ref.lat.value
    property real home_lon: mandala.est.ref.lon.value


    coordinate: QtPositioning.coordinate(home_lat,home_lon)

    //constants
    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2

    sourceItem:
    MapSvgImage {
        id: image
        source: "../icons/home.svg"
        sourceSize.height: 16*map.itemsScaleFactor
        color: "#000"
        glowColor: "#0f0"
    }

}
