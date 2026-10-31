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

import KmlOverlay 1.0

MapItemGroup {
    id: control

    property var map: ui.map
    property var plugin: apx.tools.kmloverlay
    property var kmlCenter: plugin.center
    onKmlCenterChanged: {
        if (map && kmlCenter.isValid)
            map.centerOn(kmlCenter)
    }

    function refresh(force)
    {
        if (!map || !map.visibleRegion.isValid)
            return
        if (!plugin.show.value)
            return
        raster.scheduleRender(map.visibleRegion, map.zoomLevel, force === true)
    }

    // zooming and resizing are debounced, the texture is scaled meanwhile
    Timer {
        id: updateTimer
        interval: 120
        repeat: false
        onTriggered: control.refresh(false)
    }

    Connections {
        target: control.map

        function onCenterChanged() {
            // panning inside the rendered margin needs no new image,
            // once the viewport leaves it render right away
            if (raster.covers(control.map.visibleRegion))
                updateTimer.restart()
            else
                control.refresh(false)
        }

        function onZoomLevelChanged() {
            updateTimer.restart()
        }

        function onWidthChanged() {
            updateTimer.restart()
        }

        function onHeightChanged() {
            updateTimer.restart()
        }
    }

    Connections {
        target: control.plugin.show

        function onValueChanged() {
            if (control.plugin.show.value)
                control.refresh(true)
        }
    }

    MapQuickItem {
        id: layer
        z: 1
        coordinate: raster.origin
        zoomLevel: raster.renderZoom
        anchorPoint: Qt.point(0, 0)
        visible: raster.ready && control.plugin.show.value
        opacity: control.plugin.opacity.value

        sourceItem: KmlRasterItem {
            id: raster
            overlay: control.plugin
        }
    }

    Component.onCompleted: {
        if (map)
            updateTimer.start()
    }
}
