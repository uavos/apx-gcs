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

import Apx.Map.Common

MissionObject {
    id: taxiwayItem

    readonly property int m_twidx: mandala.cmd.proc.wp.value
    readonly property int m_mode: mandala.cmd.proc.mode.value

    color: Style.cTaxiway
    textColor: "white"
    fact: modelData
    implicitZ: 32

    property bool largeDataSet: mission.missionSize>100
    shadow: interacting || active || (!largeDataSet)

    //Fact bindings
    property real f_distance: fact?fact.distance:0
    property bool f_current: (m_twidx) === num
    property bool f_taxi: m_mode === proc_mode_TAXI
    property var path: fact?fact.geoPath:0
    property real f_bearing: fact?fact.bearing:0
    property bool f_first: num === 0
    property int num: fact?fact.num:0

    //internal
    property bool showDetails: interacting || active || detailsLevel>15 || (map.metersToPixelsFactor*f_distance)>50

    property color pathColor: f_taxi?(f_current?Style.cLineGreen:"yellow"):Style.cNormal
    property int pathWidth: Math.max(1,((f_taxi&&f_current)?6:3)*ui.scale)

    contentsCenter: [
        //courese arrow
        Loader {
            active: (!f_first) && (interacting || showDetails)
            // asynchronous: true
            sourceComponent: Component {
                MultiEffect {
                    id: crsArrow
                    source: Image {
                        width: crsArrow.height
                        height: width
                        source: "../icons/waypoint-course.svg"
                    }
                    x: -width/2
                    y: height
                    z: -1
                    width: 24
                    height: width
                    transform: Rotation {
                        origin.x: crsArrow.width/2
                        origin.y: -crsArrow.width
                        axis.z: 1
                        angle: taxiwayItem.f_bearing-map.bearing
                    }
                    opacity: ui.effects?0.6:1
                    brightness: 1
                    colorization: 1
                    colorizationColor: pathColor
                }
            }
        }
    ]



    //Txi Path
    Loader {
        // asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapPolyline {
                id: polyline
                z: 10 //f_taxi?taxiwayItem.implicitZ-1:-1
                opacity: ui.effects?0.8:1
                line.width: taxiwayItem.pathWidth
                line.color: taxiwayItem.pathColor
                visible: showDetails && taxiwayItem.visible && (!f_first)
                function updatePath()
                {
                    if(taxiwayItem.path){
                        polyline.setPath(taxiwayItem.path)
                    }
                }
                Connections {
                    target: taxiwayItem
                    function onPathChanged(){ updatePath() }
                }
                Component.onCompleted: updatePath()
            }
        }
    }
}
