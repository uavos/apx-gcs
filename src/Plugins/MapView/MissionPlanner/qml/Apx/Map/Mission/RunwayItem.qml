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
import QtQuick.Shapes
import QtLocation
import QtPositioning

import Apx.Map.Common

import APX.Mission as APX


MissionObject {
    id: runwayItem

    //readonly property int m_rwidx: mandala.cmd.proc.rw.value
    readonly property int m_mode: mandala.cmd.proc.mode.value
    readonly property real m_radius: mandala.cmd.pos.radius.value
    readonly property real m_delta: mandala.est.wpt.derr.value

    color: Style.cRunway
    textColor: "white"
    title: (f_approach>0?"":"H")+(num+1)
    fact: modelData
    implicitZ: 60

    //Fact bindings
    property int f_type: fact?fact.type.value:0
    property string typeText: fact?fact.type.text:""
    property string titleText: fact?fact.title:""
    property int f_approach: fact?fact.approach.value:0
    property int f_hmsl: fact?fact.hmsl.value:0
    property real f_heading: fact?fact.heading:0
    property var endPointCoordinate: (fact && fact.endPoint)?fact.endPoint:coordinate
    property var appPointCoordinate: (fact && fact.appPoint)?fact.appPoint:coordinate
    property int num: fact?fact.num:0

    readonly property bool is_current: fact?fact.active:false
    readonly property bool is_landing: m_mode === proc_mode_LANDING && is_current

    function updateEndPoint(coord)
    {
        fact.endPoint=coord
    }
    function updateAppPoint(coord)
    {
        fact.appPoint=coord
    }

    //internal
    property bool showDetails: detailsLevel>13
    property string rwName: titleText.indexOf(' ')>0?titleText.split(' ')[1]:titleText


    contentsRight: [
        Loader {
            active: showDetails && ((!dragging)?((hover||selected)?1:(ui.effects?0.6:1)):0)
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: is_current?Style.cBlue:Style.cNormal
                    text: rwName
                }
            }
        },
        Loader {
            active: showDetails && f_hmsl!=0 && ((!dragging)?((hover||selected)?1:(ui.effects?0.6:1)):0)
            // asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cGreen
                    text: f_hmsl+"m"
                }
            }
        }
    ]



    //Runway Map Items
    property Item appPoint
    property color cRwBG: is_landing?"blue":Style.cBlue
    property bool showDetailsRw: runwayItem.visible && detailsLevel>15
    property bool showDetailsApp: runwayItem.visible && detailsLevel>10
    property real appOpacity: ui.effects?(is_landing?1:0.6):1

    property bool appHover: fact?(hover||(appPoint && appPoint.hover)||dragging||(appPoint && appPoint.dragging)):false
    property bool appCircleActive: is_landing||appHover
    property bool appCircleVisible: runwayItem.visible && showDetailsApp && (f_approach>0||appHover)
    property int appCircleLineWidth: Math.max(1,(appCircleActive?2:1)*ui.scale)
    property real appCircleOpacity: ui.effects?(appCircleActive?0.8:0.6):1

    property variant appCircleAppCoord: appPointCoordinate
    property variant appCircleCoordinate: fact?appCircleAppCoord.atDistanceAndAzimuth(appCircleRadius,f_heading+(f_type===0?-90:90)):coordinate
    //TA circle and the flight path to it, estimated from the autopilot config (shiva.perf.radius)
    property variant appCircleCoordinateDefault: fact?fact.taCenter:coordinate
    property real appCircleRadiusDefault: fact?fact.taRadius:0
    property var taPath: fact?fact.taPath.path:[]
    property real appCircleRadius: Math.max(100,is_landing?Math.abs(m_radius):appCircleRadiusDefault)

    Component.onCompleted: {
        var c
        //c=createMapComponent(endPointC)
        //c=createMapComponent(appPointC)
        //appPoint=c
        //runway path
        c=createMapComponent(pathC)
        c.z=+10 //map.z+10
        c.line.width=Math.max(1,10*ui.scale)
        c.line.color=Qt.binding(function(){return cRwBG})
        c.opacity=ui.effects?0.9:1
        c.visible=Qt.binding(function(){return is_current && runwayItem.visible})

        c=createMapComponent(pathC)
        c.z=+11
        c.line.width=Math.max(1,4*ui.scale)
        c.line.color="white"
        c.opacity=ui.effects?0.8:1

        c=createMapComponent(pathC)
        c.z=+12
        c.line.width=Math.max(1,2*ui.scale)
        c.line.color="black"
        c.opacity=ui.effects?0.5:1
        c.visible=Qt.binding(function(){return showDetailsRw})

        //approach path
        c=createMapComponent(pathC)
        c.z=+1
        c.line.width=Qt.binding(function(){return appCircleLineWidth})
        c.line.color="white"
        c.opacity=Qt.binding(function(){return appOpacity})
        c.visible=Qt.binding(function(){return showDetailsApp})
        c.p2=Qt.binding(function(){return appPointCoordinate})

        //approach circle
        c=createMapComponent(circleC)
        c.z=+1
        c.border.color="white"
        c.border.width=Qt.binding(function(){return appCircleLineWidth})
        c.opacity=Qt.binding(function(){return appCircleOpacity})
        c.visible=Qt.binding(function(){return appCircleVisible})
        c.center=Qt.binding(function(){return appCircleCoordinateDefault})
        c.radius=Qt.binding(function(){return appCircleRadiusDefault})

        //delta rect
        c=createMapComponent(deltaC)
        c.z=1
    }

    //handles
    Loader {
        //appPoint
        // asynchronous: true
        active: fact
        onLoaded: {
            map.addMapItem(item)
            appPoint=item
        }
        sourceComponent: Component {
            MissionObject {
                implicitZ: runwayItem.implicitZ-1
                visible: runwayItem.visible && showDetailsApp
                color: "white"
                textColor: "black"
                title: f_approach>0?(apx.distanceToString(f_approach)):"H----"
                property double r: runwayItem.f_heading-90;
                rotation: f_approach>0?apx.angle90(r):apx.angle(r);
                implicitCoordinate: appPointCoordinate
                onMoved: {
                    updateAppPoint(coordinate)
                    coordinate=implicitCoordinate //snap
                }
            }
        }
    }
    Loader {
        //endPoint
        // asynchronous: true
        active: fact
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MissionObject {
                id: endPoint
                implicitZ: runwayItem.implicitZ-1
                color: "white"
                textColor: "black"
                //opacity: ui.effects?0.8:1
                title: runwayItem.title
                implicitCoordinate: endPointCoordinate
                onMoved: updateEndPoint(coordinate)
                contentsRight: [
                    Loader {
                        active: showDetails && ((dragging||hover||selected)?1:0)
                        // asynchronous: true
                        sourceComponent: Component {
                            MapText {
                                textColor: "white"
                                color: is_current?Style.cBlue:Style.cNormal
                                text: runwayItem.rwName + " ("+Math.floor(apx.angle360(f_heading)).toFixed()+")"
                            }
                        }
                    }
                ]
            }
        }
    }

    //estimated flight path from the last waypoint to the TA circle (dashed, screen space)
    Loader {
        // asynchronous: true
        active: fact
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapQuickItem {
                id: taLine
                z: runwayItem.implicitZ-10
                // shown for the active runway, the selected one or when there is nothing to choose from
                visible: runwayItem.visible && runwayItem.showDetailsApp && runwayItem.taPath.length>=2
                         && (runwayItem.is_current || runwayItem.selected || mission.rw.size===1)
                opacity: ui.effects?0.9:1

                readonly property var p1: runwayItem.taPath.length>=2 ? runwayItem.taPath[0] : QtPositioning.coordinate()
                readonly property var p2: runwayItem.taPath.length>=2 ? runwayItem.taPath[1] : QtPositioning.coordinate()

                // screen vector from p1 to p2, re-evaluated on map view changes
                readonly property point d: {
                    var deps = [map.zoomLevel, map.bearing, map.tilt, map.center]
                    if(!visible || !p1.isValid || !p2.isValid)
                        return Qt.point(0,0)
                    var s1 = map.fromCoordinate(p1,false)
                    var s2 = map.fromCoordinate(p2,false)
                    return Qt.point(s2.x-s1.x, s2.y-s1.y)
                }
                readonly property real lineWidth: Math.max(1,3*ui.scale)
                readonly property real margin: lineWidth*2

                // dash length is fixed in meters (relative to the circle radius),
                // so the pattern scales with the map and does not jump on zoom
                readonly property real lengthM: (p1.isValid && p2.isValid) ? p1.distanceTo(p2) : 0
                readonly property real pxPerM: lengthM>0 ? Math.hypot(d.x,d.y)/lengthM : 0
                readonly property real dashPx: Math.max(3, runwayItem.appCircleRadiusDefault/8*pxPerM)

                coordinate: p1
                anchorPoint.x: (d.x<0 ? -d.x : 0) + margin
                anchorPoint.y: (d.y<0 ? -d.y : 0) + margin

                sourceItem: Shape {
                    width: Math.abs(taLine.d.x) + taLine.margin*2
                    height: Math.abs(taLine.d.y) + taLine.margin*2
                    preferredRendererType: Shape.CurveRenderer
                    ShapePath {
                        strokeWidth: taLine.lineWidth
                        // blue for the current (active) runway, orange for another selected one
                        strokeColor: runwayItem.is_current ? "#1E50C8" : "#FF8C00"
                        strokeStyle: ShapePath.DashLine
                        // dash pattern is in stroke width units
                        dashPattern: [taLine.dashPx/taLine.lineWidth, taLine.dashPx*0.6/taLine.lineWidth]
                        fillColor: "transparent"
                        startX: taLine.anchorPoint.x
                        startY: taLine.anchorPoint.y
                        PathLine {
                            x: taLine.anchorPoint.x + taLine.d.x
                            y: taLine.anchorPoint.y + taLine.d.y
                        }
                    }
                }
            }
        }
    }


    //paths
    Component {
        id: pathC
        MapLine {
            id: line
            visible: runwayItem.visible
            p1: runwayItem.coordinate
            p2: runwayItem.endPointCoordinate
        }
    }
    Component {
        id: circleC
        MapCircle {
            id: circle
            color: "transparent"
            visible: runwayItem.visible
        }
    }

    //delta rect polygon
    Component {
        id: deltaC
        MapPolygon {
            id: delta
            visible: runwayItem.visible && is_landing && v!=0

            property int v: isFinite(m_delta)?m_delta:0
            Behavior on v { enabled: ui.smooth; NumberAnimation {duration: 100; } }

            property var p1: runwayItem.coordinate.atDistanceAndAzimuth(25,f_heading-90)
            property var p2: runwayItem.coordinate.atDistanceAndAzimuth(25,f_heading+90)
            property var p3: p2.atDistanceAndAzimuth(v,f_heading)
            property var p4: p1.atDistanceAndAzimuth(v,f_heading)

            path: runwayItem.coordinate.isValid?[ p1, p2, p3, p4 ]:[]

            readonly property bool warn: v<-300
            color: warn?"#60FF0000":(v>=150||v<-50)?"#40FFFF00":"#4000FF00"
            Behavior on color { enabled: ui.smooth; ColorAnimation {duration: 200; } }
            border.color: warn?"#FF0000":"#00FF00"
            border.width: 1
        }
    }
}
