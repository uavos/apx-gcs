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
    id: geoItem

    color: fact?fact.opts.color:"#FF8000"
    textColor: "white"
    fact: modelData
    implicitZ: 30
    opacity: (hover || selected)?1:ui.effects?0.5:1
    title: fact?fact.title:"GEO"

    //Fact bindings
    readonly property int m_radius: Math.abs(fact?fact.radius.value:0)
    readonly property string m_role: fact?fact.role.text:""
    
    readonly property var polygon: fact?fact.polygon:0
    readonly property var pointsModel: fact?fact.pointsModel:0
    readonly property var polyC1: fact?fact.polyC1:QtPositioning.coordinate()
    readonly property var polyC2: fact?fact.polyC2:QtPositioning.coordinate()

    readonly property bool showBG: m_role!=="safe"

    // Shapes
    readonly property real shapeOpacity: (selected||dragging||hover)?0.5:showBG?0.2:0.5
    readonly property real pathWidth: 8
    readonly property color bgColor: Qt.lighter(color, 1.9)
    
    readonly property bool isCircle: fact && fact.shape.text==="circle"
    readonly property bool isPolygon: fact && fact.shape.text==="polygon"
    readonly property bool isLine: fact && fact.shape.text==="line"


    readonly property bool editActive: fact?fact.active:false

    // Circle
    readonly property var radiusPointCoordinate: fact?fact.radiusPoint:QtPositioning.coordinate()
    Loader {
        active: geoItem.isCircle
        onLoaded: map.addMapItemGroup(item)
        sourceComponent: Component {
            MapItemGroup {
                MapCircle {
                    color: showBG?geoItem.bgColor:"transparent"
                    border.color: geoItem.color
                    border.width: geoItem.pathWidth
                    opacity: geoItem.shapeOpacity
                    radius: geoItem.m_radius
                    center: geoItem.coordinate
                    Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
                }
                MissionObject { // circle handle
                    implicitZ: geoItem.implicitZ-1
                    color: "white"
                    textColor: "black"
                    title: apx.distanceToString(m_radius)
                    opacity: (geoItem.hover || geoItem.selected || selected)?(ui.effects?0.8:1):0
                    visible: opacity>0
                    implicitCoordinate: radiusPointCoordinate
                    interactive: true
                    onMoved: {
                        geoItem.fact.radiusPoint=coordinate
                        coordinate=implicitCoordinate //snap
                    }
                }
            }
        }
    }
    
    // Line
    readonly property var linePointCoordinate: fact?fact.p2.coordinate:QtPositioning.coordinate()
    Loader {
        active: geoItem.isLine
        onLoaded: map.addMapItemGroup(item)
        sourceComponent: Component {
            MapItemGroup {
                MapPolyline {
                    // color: showBG?geoItem.bgColor:"transparent"
                    line.color: geoItem.color
                    line.width: geoItem.pathWidth
                    opacity: geoItem.shapeOpacity
                    path: [ geoItem.coordinate, geoItem.linePointCoordinate ]
                }
                MissionObject { // line handle
                    implicitZ: geoItem.implicitZ-1
                    color: "white"
                    textColor: "black"
                    title: apx.distanceToString(geoItem.coordinate.distanceTo(geoItem.linePointCoordinate))
                    opacity: (geoItem.hover || geoItem.selected || selected)?(ui.effects?0.8:1):0
                    visible: opacity>0
                    implicitCoordinate: linePointCoordinate
                    interactive: true
                    onMoved: {
                        geoItem.fact.p2.coordinate=coordinate
                        coordinate=implicitCoordinate //snap
                    }
                }
            }
        }
    }
    

    // Polygon
    Loader {
        active: isPolygon
        onLoaded: map.addMapItemGroup(item)
        sourceComponent: Component {
            MapItemGroup {
                z: geoItem.implicitZ
                MapPolygon {
                    id: poly
                    // visible: geoItem.visible
                    opacity: geoItem.shapeOpacity
                    color: geoItem.showBG?geoItem.bgColor:"transparent"
                    border.width: geoItem.pathWidth
                    border.color: geoItem.color
                    path: geoItem.polygon.perimeter
                }

                // polygon vertex points
                MapItemView {
                    id: pointInst
                    property bool pointSelect: false
                    model: geoItem.pointsModel
                    delegateModelAccess: DelegateModel.ReadOnly
                    delegate: MapObject {
                        radiusFactor: 2
                        color: geoItem.color
                        title: (hover||selected)?modelData.num+1:""
                        opacity: ui.effects?0.8:1
                        visible: geoItem.editActive
                        implicitCoordinate: modelData.coordinate
                        onMoved: modelData.coordinate=coordinate
                        onSelectedChanged: {
                            modelData.active=selected
                            if(selected) selectedPointIndex=modelData.num
                        }
                        Component.onCompleted: {
                            if(pointInst.pointSelect){
                                select()
                                pointInst.pointSelect=false
                            }
                        }
                        // press and hold on selected point to delete it
                        onPressAndHold: {
                            if(geoItem.pointsModel.count>3){
                                console.log("Remove point #"+(modelData.num+1))
                                geoItem.fact.removePoint(modelData.num)
                                geoItem.select()
                            }
                        }
                    }
                }

                // items visible when at least one point is selected
                property int selectedPointIndex: -1
                Connections {
                    target: geoItem
                    function onSelectedChanged(){ selectedPointIndex=-1; }
                }

                // two extra points for adding new vertex
                Repeater {
                    model: 2
                    delegate: MapObject {
                        implicitZ: -1
                        readonly property bool prev: model.index===0
                        radiusFactor: 2
                        color: geoItem.color
                        title: "+"
                        opacity: ui.effects?0.8:1
                        visible: geoItem.editActive && selectedPointIndex>=0
                        implicitCoordinate: prev?geoItem.polyC1:geoItem.polyC2
                        function addPoint()
                        {
                            // console.log("Add point at "+coordinate)
                            var i=selectedPointIndex
                            pointInst.pointSelect=true
                            if(!prev)
                                selectedPointIndex++;
                            geoItem.fact.addPoint(coordinate, prev?i:(i+1))
                        }
                        // override default interactive item behavior
                        interactive: false
                        MouseArea {
                            cursorShape: Qt.PointingHandCursor
                            anchors.fill: parent
                            onClicked: parent.addPoint() 
                        }
                    }
                }
            }
        }
    }


}
