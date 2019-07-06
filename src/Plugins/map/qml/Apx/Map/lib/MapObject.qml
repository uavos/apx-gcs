import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtQml 2.12
import QtGraphicalEffects 1.12

import "../lib"

MapQuickItem {  //to be used inside MapComponent only
    id: mapObject

    property string title
    property bool interactive: visibleOnMap
    property bool draggable: true

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

    signal menuRequested()
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
    /*Connections {
        target: map
        onCenterChanged: updateViewportTimer.restart()
        onZoomLevelChanged: updateViewportTimer.restart()
    }
    Timer {
        id: updateViewportTimer
        interval: 1000
        repeat: false
        running: true
        onTriggered: {
            visibleOnMap=map.visibleRegion.contains(coordinate)
            updateMapViewport()
        }
    }*/

    Connections {
        target: map
        onClicked: deselect()
        onMenuRequested: deselect()
    }


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
        map.showCoordinate(mapObject.coordinate)
    }


    //internal logic
    property bool hover: mouseArea.containsMouse
    property bool dragging: mouseArea.drag.active
    property bool selected: map.selectedObject===this

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
    z: (dragging||selected||hover)?1000:mapObject.implicitZ
    width: sourceItem.width
    height: sourceItem.height

    sourceItem:
    Item {
        width: textItem.width
        height: textItem.height
        Rectangle {
            id: frame
            visible: selected
            anchors.centerIn: textItem
            width: textItem.width*textItem.scale+10
            height: textItem.height*textItem.scale+10
            antialiasing: true
            //smooth: ui.antialiasing
            border.width: 2
            border.color: "#FFFFFF"
            radius: 5
            color: "#50FFFFFF"
            opacity: ui.effects?0.6:1
        }
        MapText {
            id: textItem
            horizontalAlignment: Text.AlignHCenter
            //scale: hoverScale*map.itemsScaleFactor*missionItemsScaleFactor
            //opacity: (dragging && ui.effects)?0.6:1
            text: title
            font.pixelSize: map.fontSize * 0.8
            margins: 1
            radius: 2
            font.bold: false
            square: true
            visible: false //!ui.effects
        }
        DropShadow {
            anchors.fill: textItem
            //scale: hoverScale*map.itemsScaleFactor*missionItemsScaleFactor
            //horizontalOffset: 3
            //verticalOffset: 3
            //radius: 8
            //spread: 0.1
            samples: ui.effects?15:0
            color: (dragging||hover)?"#8f8":"#a0000000"
            source: textItem
            cached: true
            enabled: false
            visible: !textItem.visible
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
            enabled: interactive
            hoverEnabled: enabled
            cursorShape: (enabled && (!drag.active))?Qt.PointingHandCursor:Qt.ArrowCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: textItem
            scale: textItem.scale
            drag.target: mapObject.draggable?mapObject:null
            onPositionChanged: if(drag.active) objectMoving()
            onClicked: {
                if (mouse.button === Qt.LeftButton) {
                    select()
                }else if (mouse.button === Qt.RightButton) {
                    selectAndCenter()
                    menuRequested()
                }
            }
            onDoubleClicked: {
                selectAndCenter()
                menuRequested()
            }
            onPressAndHold: doubleClicked(mouse)
        }
    }

}
