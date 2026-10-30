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
import QtLocation
import QtPositioning
import QtQml

MapItemGroup {
    id: control

    property var map: ui.map
    property var area: apx.tools.missionplanner.area
    property var plugin: apx.tools.kmloverlay
    property var kmlCenter: plugin.center
    onKmlCenterChanged: {
        if (map && kmlCenter.isValid) {
            map.centerOn(kmlCenter)
            updateTimer.restart()
        }
    }

    function updateVisiblePolygons()
    {
        if (map && map.visibleRegion.isValid)
            plugin.updateKmlModels(map.visibleRegion)
    }

    Timer {
        id: updateTimer
        interval: 250
        repeat: false
        onTriggered: control.updateVisiblePolygons()
    }

    Connections {
        target: control.map

        function onCenterChanged() {
            updateTimer.restart()
        }

        function onZoomLevelChanged() {
            updateTimer.restart()
        }
    }

    Connections {
        target: control.plugin

        function onLoadingChanged() {
            if (!control.plugin.loading)
                updateTimer.restart()
        }
    }

    onAreaChanged: updateTimer.restart()

    MapItemView {
        id: borderPointsView
        model: plugin.kmlPolygons

        z: 1
        delegate: MapPolygon {
            path: polygonPath
            color: polygonColor
            opacity: plugin.opacity.value
            border.color: polygonColor
            border.width: 1
            autoFadeIn: false
        }
    }

    Component.onCompleted: {
        if (map) {
            map.addMapItemView(borderPointsView)
            updateTimer.start()
        }
    }
}
