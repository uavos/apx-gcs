import QtQuick 2.12
import QtLocation 5.13
import QtGraphicalEffects 1.0

MapQuickItem {  //to be used inside MapComponent only
    id: mapObject

    property string title
    property bool interactive: visibleOnMap
    property bool draggable: true
    property bool shadow: true

    property int implicitZ: 0
    property int radius: 2

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
        onClicked: deselect()
        onMenuRequested: deselect()
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
            asynchronous: true
            sourceComponent: Component {
                Rectangle {
                    id: frame
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
            }
        }
        MapText {
            id: textItem
            horizontalAlignment: Text.AlignHCenter
            //scale: hoverScale*map.itemsScaleFactor*missionItemsScaleFactor
            //opacity: (dragging && ui.effects)?0.6:1
            text: title
            font.pixelSize: map.fontSize * 0.8
            margins: 1
            radius: mapObject.radius
            font.bold: false
            square: true
            visible: shadowLoader.status!=Loader.Ready
        }
        Loader {
            id: shadowLoader
            anchors.fill: parent
            active: shadow
            asynchronous: true
            sourceComponent: Component {
                DropShadow {
                    //anchors.fill: textItem
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
