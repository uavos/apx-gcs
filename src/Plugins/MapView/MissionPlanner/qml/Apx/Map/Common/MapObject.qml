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

MapQuickItem {  //to be used inside MapComponent only
    id: mapObject

    property string title
    property bool interactive: visibleOnMap
    property bool draggable: true
    property bool shadow: true

    property int implicitZ: 0

    property var implicitCoordinate

    property alias color: textItem.color
    property alias text: textItem.text
    property alias font: textItem.font
    property alias textColor: textItem.textColor

    property real hoverScaleFactor: 1.5
    property real missionItemsScaleFactor: 1

    property alias contentsTop:     containerTop.children
    property alias contentsRight:   containerRight.children
    property alias contentsBottom:  containerBottom.children
    property alias contentsCenter:  containerCenter.children

    property real radiusFactor: 10


    signal triggered()
    signal moved()
    signal movingFinished()

    enabled: visibleOnMap

    property var mapItems: []
    Component.onDestruction: {
        for(var i=0;i<mapItems.length;++i){
            map.removeMapItem(mapItems[i])
            mapItems[i].destroy()
        }
        mapItems=[]
    }

    function createMapComponent(cmp)
    {
        var c=cmp.createObject(map)
        map.addMapItem(c)
        mapItems.push(c)
        return c
    }


    //optimizations
    property bool visibleOnMap: true
    signal updateMapViewport()

    Connections {
        target: map
        function onClicked(){ deselect() }
        function onMenuRequested(){ deselect() }
    }

    onTriggered: center()

    function select()
    {
        if(map.selectedObject!==mapObject) map.selectedObject=mapObject
    }
    function deselect()
    {
        if(map.selectedObject) map.selectedObject=null
    }
    function selectAndCenter()
    {
        select()
        center()
    }
    function center()
    {
        map.showCoordinate(mapObject.coordinate)
    }


    //internal logic
    property bool hover: mouseArea.containsMouse
    property bool dragging: mouseArea.drag.active
    property bool selected: map.selectedObject===this

    property bool interacting: selected || dragging || hover

    property real hoverScale: ((dragging||hover)?hoverScaleFactor:1)

    Behavior on hoverScale { enabled: ui.smooth; NumberAnimation {duration: 100; } }
    Behavior on opacity { enabled: ui.smooth; NumberAnimation {duration: 200; } }


    //dragging support
    function objectMoving()
    {
        moved()
        //flick when dragging to edges
        var p=map.fromCoordinate(coordinate);
        var ws=64;
        if(p.x<ws || p.y<ws || p.x>(map.width-ws) || p.y>(map.height-ws)){
            map.flickToCoordinate(coordinate)
        }
    }
    onDraggingChanged: {
        if(dragging){
            if(!selected) select()
        }else{
            movingFinished()
            if(implicitCoordinate) coordinate=Qt.binding(function(){return implicitCoordinate})
        }
    }

    //position
    coordinate: implicitCoordinate?implicitCoordinate:QtPositioning.coordinate()
    anchorPoint.x: textItem.width/2
    anchorPoint.y: textItem.height/2
    z: interacting?99999:mapObject.implicitZ
    width: sourceItem.width
    height: sourceItem.height

    sourceItem:
    Item {
        width: textItem.width
        height: textItem.height
        Loader {
            anchors.centerIn: textItem
            active: selected
            // asynchronous: true
            sourceComponent: Component {
                Rectangle {
                    id: frame
                    width: textItem.width*textItem.scale+10
                    height: textItem.height*textItem.scale+10
                    antialiasing: true
                    border.width: 2
                    border.color: "#FFFFFF"
                    radius: shadow?height/mapObject.radiusFactor:0
                    color: "#50FFFFFF"
                    opacity: ui.effects?0.6:1
                }
            }
        }
        MapText {
            id: textItem
            horizontalAlignment: Text.AlignHCenter
            text: title
            square: true
            radiusFactor: mapObject.radiusFactor
            visible: shadowLoader.status!=Loader.Ready
        }
        Loader {
            id: shadowLoader
            anchors.fill: parent
            active: shadow
            // asynchronous: true
            sourceComponent: Component {
                MultiEffect {
                    source: textItem
                    enabled: false
                    shadowEnabled: ui.effects
                    visible: !textItem.visible
                }
            }
        }
        Item{
            id: containerCenter
            z: -10
            visible: visibleOnMap
            anchors.centerIn: textItem
        }
        Column{
            id: containerTop
            z: -10
            visible: visibleOnMap
            spacing: 1
            anchors.bottom: textItem.top
            anchors.bottomMargin: textItem.height
            anchors.horizontalCenter: textItem.horizontalCenter
        }
        Column{
            id: containerRight
            z: -10
            visible: visibleOnMap
            spacing: 1
            anchors.left: textItem.right
            anchors.leftMargin: textItem.width
            anchors.verticalCenter: textItem.verticalCenter
        }
        Column{
            id: containerBottom
            z: -10
            visible: visibleOnMap
            spacing: 1
            anchors.top: textItem.bottom
            anchors.topMargin: textItem.height
            anchors.horizontalCenter: textItem.horizontalCenter
        }
        MouseArea {
            id: mouseArea
            enabled: interactive && !map.selectedTool
            hoverEnabled: enabled
            cursorShape: (enabled && (!drag.active))?Qt.PointingHandCursor:Qt.ArrowCursor
            //acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: textItem
            scale: textItem.scale
            drag.target: mapObject.draggable?mapObject:null
            onPositionChanged: if(drag.active) objectMoving()
            onClicked: {
                if(selected)triggered()
                else select()
            }
        }
    }

}
