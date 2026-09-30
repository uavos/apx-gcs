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

import Apx.Application

// Map layer of the plugin: requests the terrain elevation under the mouse
// cursor (shown in the map info line). Attached to the map the same way as
// the Sites and KmlOverlay plugins, the map itself knows nothing about it.
AppPlugin {
    id: plugin

    uiComponent: "map"
    onConfigure: {
        parent = ui.map
        anchors.fill = ui.map
    }

    sourceComponent: Item {
        id: layer

        HoverHandler {
            id: hover
            onPointChanged: {
                if(!hovered || timer.running)
                    return
                timer.pos = point.position
                timer.start()
            }
        }
        Timer {
            id: timer
            property point pos: Qt.point(0, 0)
            interval: 500
            onTriggered: {
                var elevationmap = apx.tools.elevationmap
                if(!ui.map || !elevationmap || !elevationmap.available || !elevationmap.use.value)
                    return
                elevationmap.setElevationByCoordinate(ui.map.toCoordinate(pos))
            }
        }
    }
}
