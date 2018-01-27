import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.FactSystem 1.0
import "."
import ".."

MapQuickItem {  //to be used inside MapComponent only
    id: missionObject

    property var fact: null

    property bool interactive: visibleOnMap

    property int implicitZ: 0

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

    visible: mission.visible //&& visibleOnMap
    enabled: visibleOnMap //visible

    property var mapItems: []
    Component.onDestruction: {
        for(var i=0;i<mapItems.length;++i)mapItems[i].destroy()
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
    }


    Connections {
        target: fact
        enabled: fact!==null
        onTriggered: selectAndCenter()
    }

    signal moved()

    function select()
    {
        if(fact) mission.select(missionObject)
    }
    function deselect()
    {
        if(fact) mission.deselect()
    }
    function showMenu()
    {
        select()
        if(fact) map.showFactMenu(fact)
    }
    function selectAndCenter()
    {
        select()
        map.centerOnItem(missionObject)
    }


    //internal logic
    property alias hover: mouseArea.containsMouse
    property bool dragging: mouseArea.drag.active
    property bool selected: map.selectedObject===this

    property real hoverScale: ((dragging||hover)?hoverScaleFactor:1)

    Behavior on hoverScale { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
    Behavior on opacity { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }


    //Fact bindings
    property var factPos: fact?fact.coordinate:QtPositioning.coordinate()
    property real altitude: (fact && fact.altitude)?fact.altitude.value:0
    property real active: fact?fact.active:0
    property string title: fact?fact.num+1:0



    //dragging support
    function objectMoving()
    {
        if(fact){
            fact.coordinate=coordinate
        }
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
            coordinate=coordinate
            if(!selected) deselect()
        }else{
            coordinate=Qt.binding(function(){return factPos})
        }
    }

    //position
    coordinate: factPos
    anchorPoint.x: textItem.width/2
    anchorPoint.y: textItem.height/2
    z: (dragging||selected||hover)?200:implicitZ
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
            smooth: true
            border.width: 2
            border.color: "#FFFFFF"
            radius: 5
            color: "#50FFFFFF"
            opacity: 0.6
        }
        MapText {
            id: textItem
            horizontalAlignment: Text.AlignHCenter
            scale: hoverScale*map.itemsScaleFactor*missionItemsScaleFactor
            opacity: dragging?0.6:1
            text: title
            font.pixelSize: Qt.application.font.pixelSize * 0.8
            margins: 1
            radius: 2
            font.bold: false
            square: true
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
            drag.target: missionObject
            onPositionChanged: if(drag.active) objectMoving()
            onClicked: {
                if (mouse.button === Qt.LeftButton) {
                    select()
                }else if (mouse.button === Qt.RightButton) {
                    selectAndCenter()
                    showMenu()
                }
            }
            onDoubleClicked: {
                selectAndCenter()
                showMenu()
            }
            onPressAndHold: doubleClicked(mouse)
        }
    }

}
