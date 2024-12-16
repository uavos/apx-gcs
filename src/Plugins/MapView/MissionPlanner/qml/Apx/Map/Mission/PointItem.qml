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

import Apx.Map.Common

MissionObject {
    id: pointItem

    readonly property int m_piidx: mandala.cmd.proc.pi.value


    color: Style.cPoint
    textColor: "white"
    fact: modelData
    implicitZ: 40

    //Fact bindings
    property int f_radius: Math.abs(fact?fact.radius.value:0)
    property int f_hmsl: fact?fact.hmsl.value:0
    property int f_loops: fact?fact.loops.value:0
    property int f_time: fact?fact.timeout.value:0
    property string timeText: fact?fact.timeout.text:""
    property var radiusPointCoordinate: fact?fact.radiusPoint:QtPositioning.coordinate()
    property bool f_ccw: fact?fact.radius.value<0:false
    property int num: fact?fact.num:0

    property bool current: m_piidx === num

    function updateRadiusPoint(coord)
    {
        fact.radiusPoint=coord
    }

    //internal
    property bool showDetails: detailsLevel>13

    contentsTop: [
        Loader {
            // Feets
            property var opts: fact?fact.hmsl.opts:0
            property int feets: !isNaN(parseInt(opts.ft))?opts.ft:0
            property bool isFeets: fact?fact.isFeets:false

            active: (f_hmsl!==0 || feets !==0) && ((!dragging)?((hover||selected)?1:(showDetails?(ui.effects?0.6:1):0)):0)
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cGreen
                    text: !isFeets ? f_hmsl+"m" : feets + "ft"
                }
            }
        }
    ]

    contentsBottom: [
        Loader {
            active: (f_loops>0 || f_time>0) && ((!dragging)?((hover||selected)?1:(showDetails?(ui.effects?0.6:1):0)):0)
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cNormal
                    text: (f_loops>0?"L"+f_loops:"")+
                          (f_time>0?"T"+timeText:"")
                }
            }
        }
    ]


    //Map Items
    property bool circleActive: selected||dragging||hover||radiusPointSelected
    property Item radiusPoint
    property bool radiusPointSelected: radiusPoint && radiusPoint.selected

    Component.onCompleted: {
        //createMapComponent(lineC)
        //createMapComponent(circleC)
        //radiusPoint=createMapComponent(radiusPointC)
    }
    Loader {
        // Feets
        property var opts: fact?fact.radius.opts:0
        property bool isFeets: fact?fact.isFeets:false
        property int radius: pointItem.f_radius
        property var poiTitle: getTitle()

        onIsFeetsChanged: poiTitle=getTitle()
        onOptsChanged: poiTitle=getTitle()
        onRadiusChanged: poiTitle=getTitle()

        function getTitle() {
            if(isFeets)
                return opts.ft>0?(apx.distanceToStringFt(opts.ft)):"H----"
            else
                return f_radius>0?(apx.distanceToString(f_radius)):"H----"
        }

        //handle
        // asynchronous: true
        onLoaded: {
            map.addMapItem(item)
            radiusPoint=item
        }
        sourceComponent: Component {
            MissionObject {
                implicitZ: pointItem.implicitZ-1
                color: "white"
                textColor: "black"
                title: poiTitle
                opacity: (pointItem.hover || pointItem.selected || selected)?(ui.effects?(f_radius>0?0.8:0.5):1):0
                visible: opacity>0
                implicitCoordinate: radiusPointCoordinate
                interactive: pointItem.selected || selected
                onMoved: {
                    updateRadiusPoint(coordinate)
                    coordinate=implicitCoordinate //snap
                }
                //direction arrow
                contentsTop: [
                    MultiEffect {
                        id: crsArrow1
                        //z: map.z
                        width: 24
                        height: width
                        opacity: ui.effects?0.8:1
                        brightness: 1
                        visible: f_ccw
                        colorization: showDetails || circleActive
                        colorizationColor: Style.cPoint
                        source: Image {
                            width: crsArrow1.height
                            height: width
                            source: "../icons/waypoint-course.svg"
                        }
                    }
                ]
                contentsBottom: [
                    MultiEffect {
                        id: crsArrow2
                        //z: map.z
                        width: 24
                        height: width
                        rotation: 180
                        opacity: ui.effects?0.8:1
                        brightness: 1
                        visible: !f_ccw
                        colorization: showDetails || circleActive
                        colorizationColor: Style.cPoint
                        source: Image {
                            width: crsArrow2.height
                            height: width
                            source: "../icons/waypoint-course.svg"
                        }
                    }
                ]

            }
        }
    }
    Loader {
        // asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapCircle {
                //z: -100
                color: circleActive?"#2040FF40":"#100000FF"
                border.color: "#500000FF"
                border.width: 1
                radius: pointItem.f_radius
                center: pointItem.coordinate
                Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
            }
        }
    }
    Loader {
        // asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapLine {
                //z: -10
                visible: circleActive
                //opacity: ui.effects?radiusPoint.opacity/2:1
                line.width: 2
                p1: pointItem.coordinate
                p2: pointItem.radiusPointCoordinate
            }
        }
    }

}
