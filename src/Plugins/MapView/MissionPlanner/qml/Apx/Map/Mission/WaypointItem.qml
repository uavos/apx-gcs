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

import APX.Mission as APX


MissionObject {
    id: waypointItem
    color: visibleOnMap?Style.cWaypoint:"yellow"
    textColor: "black"
    fact: modelData
    implicitZ: 50


    property bool largeDataSet: mission.missionSize>100
    shadow: interacting || active || (!largeDataSet)

    //Fact bindings
    property real f_distance: fact?fact.distance:0
    property real f_totalDistance: fact?fact.totalDistance:0
    property int f_time: fact?fact.time:0
    property int f_totalTime: fact?fact.totalTime:0
    property string actionsText: fact?fact.actions.text:""
    property real f_bearing: fact?fact.bearing:0
    property bool f_first: num === 0
    property bool f_warning: fact?fact.warning:false
    property bool f_reachable: fact?fact.reachable:false
    property int f_type: fact?fact.type.value:0
    property var path: fact?fact.geoPath:0
    property int num: fact?fact.num:0


    //internal
    property color cReachable: f_warning?Style.cLineYellow:f_type===0?Style.cLineCyan:Style.cLineBlue
    property color pathColor: f_first?Style.cLineGreen: f_reachable?cReachable:Style.cLineRed
    property real pathWidth: Math.max(1,(f_first?2:4)*ui.scale)


    property bool showDetails: interacting || active || f_distance===0 || (map.metersToPixelsFactor*f_distance)>150

    //property bool pathVisibleOnMap: true
    //property Item pathItem
    //visible: mission.visible && (visibleOnMap || pathVisibleOnMap)
    onUpdateMapViewport: {
        //console.log(typeof(path))
        //if(fact && path)pathVisibleOnMap=map.visibleRegion.boundingGeoRectangle().intersects(path.boundingGeoRectangle())
    }

    contentsTop: [
        Loader {
            active: dragging||hover
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cNormal
                    text: apx.distanceToString(f_distance)+"/"+apx.distanceToString(f_totalDistance)
                }
            }
        },
        Loader {
            active: dragging||hover
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cNormal
                    text: (f_time>=120?apx.timeToString(f_time)+"/":"")+apx.timeToString(f_totalTime)
                }
            }
        }
    ]
    contentsRight: [
        Loader {
            active: (!dragging) && (hover||selected)?1:0
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cGreen
                    text: f_altitude.toFixed()+"m"
                }
            }
        }
    ]
    contentsBottom: [
        Loader {
            active: actionsText.length>0 && ((!dragging)?((hover||selected)?1:((detailsLevel>13 && showDetails)?(ui.effects?0.6:1):0)):0)
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cRed
                    text: actionsText
                }
            }
        }
    ]

    contentsCenter: [
        //courese arrow
        Loader {
            active: waypointItem.active || interacting //&& showDetails
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
                        angle: waypointItem.f_bearing-map.bearing
                    }
                    opacity: ui.effects?0.6:1
                    brightness: 1
                    colorization: 1
                    colorizationColor: pathColor
                }
            }
        }
    ]

    //Flight Path
    Loader {
        // asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapPolyline {
                id: polyline
                z: 50 //-90 //waypointItem.implicitZ-1
                visible: waypointItem.visible
                opacity: ui.effects?0.6:1
                //smooth: ui.antialiasing
                line.width: waypointItem.pathWidth
                line.color: waypointItem.pathColor
                function updatePath()
                {
                    if(waypointItem.path){
                        polyline.setPath(waypointItem.path)
                    }
                }
                Connections {
                    target: waypointItem
                    function onPathChanged(){ updatePath() }
                }
                Component.onCompleted: updatePath()
            }
        }
    }


}
