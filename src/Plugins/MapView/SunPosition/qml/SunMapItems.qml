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
import QtQuick.Effects
import QtLocation
import QtPositioning

import Apx.Common

MapItemGroup {
    id: group
    z: 190

    property var map: ui.map
    readonly property var plugin: apx.tools.sunposition
    readonly property var unit: apx.fleet.current

    readonly property real m_azimuth: plugin.sunAzimuth
    readonly property real m_altitude: plugin.sunAltitude

    // colors by sun elevation
    readonly property color cDay: "#FFD54F"
    readonly property color cLow: "#FF9800"   // glare risk near horizon
    readonly property color cNight: "#90A4AE"
    readonly property color color: m_altitude < 0 ? cNight : (m_altitude < 15 ? cLow : cDay)

    MapQuickItem {
        id: sunItem

        visible: plugin.show.value && plugin.sunValid && unit && unit.visible

        // follow the unit icon position (smooth)
        property var coord: unit ? unit.coordinate : QtPositioning.coordinate()
        onCoordChanged: {
            if(!coord.isValid) return
            if(!coordinate.isValid || map.isFarMove(coordinate,coord,2)){
                anim.stop()
                coordinate=coord
                return
            }
            if(!anim.running)anim.from=coordinate
            anim.to=coord
            anim.start()
        }
        Component.onCompleted: if(coord.isValid) coordinate=coord
        CoordinateAnimation {
            id: anim
            duration: ui.smooth?500:0
            direction: CoordinateAnimation.Shortest
            easing.type: Easing.Linear
            target: sunItem
            property: "coordinate"
        }

        property real vaz: m_azimuth
        Behavior on vaz { enabled: ui.smooth; RotationAnimation {duration: 500; direction: RotationAnimation.Shortest; } }

        readonly property real sz: 48*map.itemsScaleFactor*ui.scale // unit icon size
        readonly property real r0: sz*0.8   // ray start
        readonly property real r1: sz*2.6   // ray end
        readonly property real size: (r1+sz*2)*2
        readonly property real angle: vaz-map.bearing
        readonly property real rad: angle*Math.PI/180

        anchorPoint.x: size/2
        anchorPoint.y: size/2

        sourceItem: Item {
            id: content
            width: sunItem.size
            height: width
            opacity: m_altitude < 0 ? 0.6 : 0.9

            layer.enabled: ui.effects
            layer.effect: MultiEffect {
                shadowEnabled: true
            }

            // dashed ray from the unit to the sun
            Item {
                id: ray
                x: content.width/2
                y: content.height/2
                rotation: sunItem.angle
                readonly property int count: 7
                readonly property real step: (sunItem.r1-sunItem.r0)/count
                Repeater {
                    model: ray.count
                    Rectangle {
                        width: Math.max(2,sunItem.sz*0.05)
                        height: ray.step*0.6
                        radius: width/2
                        x: -width/2
                        y: -(sunItem.r0+ray.step*(index+1))
                        color: group.color
                    }
                }
            }

            MaterialIcon {
                id: icon
                name: m_altitude < 0 ? "weather-night" : "white-balance-sunny"
                size: sunItem.sz*0.7
                color: group.color
                readonly property real r: sunItem.r1+size*0.5
                x: content.width/2+Math.sin(sunItem.rad)*r-width/2
                y: content.height/2-Math.cos(sunItem.rad)*r-height/2

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: plugin.trigger()
                }
            }

            Text {
                id: label
                // on the far side of the icon, not over the ray
                x: icon.x+(icon.width-width)/2
                y: Math.cos(sunItem.rad)>0 ? icon.y-height : icon.y+icon.height
                color: group.color
                font: apx.font_narrow(sunItem.sz*0.3)
                text: "AZ " + m_azimuth.toFixed(0) + "° EL " + m_altitude.toFixed(0) + "°"
            }

            transform: Rotation {
                origin.x: content.width/2
                origin.y: content.height/2
                axis.x: 1
                axis.z: 0
                angle: map.tilt
            }
        }
    }
}
