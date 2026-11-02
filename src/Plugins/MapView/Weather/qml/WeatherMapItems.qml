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

MapItemGroup {
    id: group

    property var map: ui.map
    property var weather: apx.tools.weather

    z: 40
    visible: weather.active

    // route points: wind arrows with the info panel on hover.
    // The model thins the points out by the stride: every 2nd, 4th... point when zoomed out
    MapItemView {
        id: pointsView
        model: weather.points
        incubateDelegates: false // all the items in one go, no half-built items on the map
        delegate: WeatherItem { }
    }

    Binding {
        target: weather
        property: "stride"
        value: Math.max(1, Math.pow(2, Math.floor(15 - map.zoomLevel)))
    }

    Component.onCompleted: {
        map.addMapItemView(pointsView)
    }
}
