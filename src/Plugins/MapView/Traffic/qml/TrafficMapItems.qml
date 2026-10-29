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
    property var traffic: apx.tools.traffic

    z: 150
    visible: traffic.active

    // airspaces (ATC)
    MapItemView {
        id: airspaceView
        model: traffic.airspaces
        delegate: MapPolyline {
            path: model.path // closed ring, a polyline is cheaper than a polygon fill
            line.color: model.color
            line.width: 1.5
            opacity: 0.8
        }
    }
    MapItemView {
        id: airspaceLabels
        model: traffic.airspaces
        delegate: MapQuickItem {
            coordinate: model.center
            zoomLevel: 0
            anchorPoint.x: lbl.width/2
            anchorPoint.y: lbl.height/2
            sourceItem: Text {
                id: lbl
                text: model.label
                color: model.color
                font: apx.font_narrow(Style.fontSize*0.8)
                style: Text.Outline
                styleColor: "#000"
            }
        }
    }

    // aircraft (ADS-B)
    MapItemView {
        id: aircraftView
        model: traffic.aircraft
        delegate: TrafficItem { }
    }

    Component.onCompleted: {
        map.addMapItemView(airspaceView)
        map.addMapItemView(airspaceLabels)
        map.addMapItemView(aircraftView)
        traffic.setMapCenter(map.center)
    }

    Connections {
        target: map
        function onCenterChanged(){ traffic.setMapCenter(map.center) }
    }
}
