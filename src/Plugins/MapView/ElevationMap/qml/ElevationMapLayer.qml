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

import Apx.Common
import Apx.Application

// Map layer of the plugin: requests the terrain elevation under the mouse
// cursor (shown in the map info line) and shows the AGL next to the current
// unit. Attached to the map the same way as the Sites and KmlOverlay plugins,
// the map itself knows nothing about it.
AppPlugin {
    id: plugin

    uiComponent: "map"
    onConfigure: {
        parent = ui.map
        anchors.fill = ui.map
    }
    onLoaded: if(ui.map) ui.map.addMapItemGroup(item.mapItems)

    sourceComponent: Item {
        id: layer
        property alias mapItems: mapItems

        // height above terrain next to the current unit (plugin option "Unit AGL")
        MapItemGroup {
            id: mapItems
            z: 200 // same layer as the units
            MapQuickItem {
                id: aglItem
                readonly property real agl: plugin.fact ? plugin.fact.unitAgl : NaN
                readonly property var unit: apx.fleet.current
                // to the left of the unit icon: its own label is on the right or below
                readonly property real iconHalf: 24 * ui.scale
                                                 * ((ui.map && ui.map.itemsScaleFactor) ? ui.map.itemsScaleFactor : 1)

                visible: !isNaN(agl) && unit && unit.coordinate.isValid

                // moves exactly as the unit icon does (UnitItem): smooth, jumps on a far move
                readonly property var coord: unit ? QtPositioning.coordinate(unit.coordinate.latitude, unit.coordinate.longitude)
                                                  : QtPositioning.coordinate()
                onCoordChanged: {
                    if(!ui.map || !coordinate.isValid || ui.map.isFarMove(coordinate, coord, 2)) {
                        anim.stop()
                        coordinate = coord
                        return
                    }
                    if(!anim.running) anim.from = coordinate
                    anim.to = coord
                    anim.start()
                }
                Component.onCompleted: coordinate = coord
                CoordinateAnimation {
                    id: anim
                    duration: ui.smooth ? 500 : 0
                    direction: CoordinateAnimation.Shortest
                    easing.type: Easing.Linear
                    target: aglItem
                    property: "coordinate"
                }
                anchorPoint.x: aglText.width + iconHalf + Style.spacing
                anchorPoint.y: aglText.height / 2
                sourceItem: Text {
                    id: aglText
                    text: "AGL" + Math.round(aglItem.agl)
                    font: apx.font_narrow(Style.fontSize * 0.8)
                    color: "#fff"
                    style: Text.Outline
                    styleColor: "#000"
                }
            }
        }

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
                var elevationmap = plugin.fact
                if(!ui.map || !elevationmap || !elevationmap.available || !elevationmap.use.value)
                    return
                elevationmap.setElevationByCoordinate(ui.map.toCoordinate(pos))
            }
        }
    }
}
