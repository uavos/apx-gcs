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
    
    readonly property var path: fact?fact.geoPath:0
    readonly property var pointsModel: fact?fact.points.model:0

    readonly property bool showBG: m_role!=="safe"

    // Shapes
    readonly property real shapeOpacity: (selected||dragging||hover)?0.5:showBG?0.2:0.5
    readonly property real pathWidth: 8
    readonly property color bgColor: Qt.lighter(color, 1.9)
    
    readonly property bool isCircle: fact && fact.shape.text==="circle"
    readonly property bool isPolygon: fact && fact.shape.text==="polygon"
    readonly property bool isLine: fact && fact.shape.text==="line"


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
                    opacity: (geoItem.hover || geoItem.selected || selected)?(ui.effects?(m_radius>0?0.8:0.5):1):0
                    visible: opacity>0
                    implicitCoordinate: geoItem.fact?geoItem.fact.radiusPoint:QtPositioning.coordinate()
                    interactive: true
                    onMoved: {
                        geoItem.fact.radiusPoint=coordinate
                        coordinate=implicitCoordinate //snap
                    }
                }
            }
        }
    }

    // Polygon
    Loader {
        active: geoItem.isPolygon
        onLoaded: map.addMapItemGroup(item)
        sourceComponent: Component {
            MapItemGroup {
                MapPolygon {
                    id: poly
                    z: 50 
                    visible: geoItem.visible
                    opacity: geoItem.shapeOpacity
                    color: geoItem.showBG?geoItem.bgColor:"transparent"
                    border.width: geoItem.pathWidth
                    border.color: geoItem.color
                    function updatePath()
                    {
                        if(geoItem.path){
                            poly.path=geoItem.path.path
                        }
                    }
                    Connections {
                        target: geoItem
                        function onPathChanged(){ updatePath() }
                    }
                    Component.onCompleted: updatePath()
                }
                Instantiator {
                    model: geoItem.pointsModel
                    delegate: MapObject {
                        implicitZ: geoItem.implicitZ-1
                        radiusFactor: 2
                        color: geoItem.color
                        title: hover?modelData.num+1:""
                        // color: "white"
                        // textColor: "black"
                        //opacity: ui.effects?0.8:1
                        // title: modelData.num+1
                        implicitCoordinate: modelData.coordinate
                        onMoved: {
                            modelData.coordinate=coordinate
                        }
                        Component.onCompleted: {
                            map.addMapItem(this)
                            // console.log("Point created: "+title+" "+coordinate)
                        }
                    }
                }
            }
        }
    }


}
