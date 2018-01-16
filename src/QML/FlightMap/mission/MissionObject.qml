import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.FactSystem 1.0
import "."
import ".."

MapQuickItem {  //to be used inside MapComponent only
    id: missionObject

    property var fact

    property bool interactive: true

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

    visible: mission.visible


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
        if(fact) map.showMenu(fact)
    }

    //internal logic
    property alias hover: mouseArea.containsMouse
    property bool dragging: mouseArea.drag.active
    property bool selected: map.selectedObject===this

    property real hoverScale: ((dragging||hover)?hoverScaleFactor:1)

    Behavior on hoverScale { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
    Behavior on opacity { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }


    //Fact bindings
    property real lat: fact?fact.latitude.value:0
    property real lon: fact?fact.longitude.value:0
    property real altitude: (fact && fact.altitude)?fact.altitude.value:0
    property real active: fact?fact.active:0
    property string title: fact?fact.num+1:0



    //dragging support
    property variant factPos: QtPositioning.coordinate(lat,lon,altitude)
    function objectMoving()
    {
        if(fact){
            fact.latitude.value=coordinate.latitude
            fact.longitude.value=coordinate.longitude
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
    z: (dragging||selected||hover)?200:0
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
        Column{
            id: containerTop
            z: -10
            spacing: 1
            anchors.bottom: textItem.top
            anchors.bottomMargin: textItem.height
            anchors.horizontalCenter: textItem.horizontalCenter
        }
        Column{
            id: containerRight
            z: -10
            spacing: 1
            anchors.left: textItem.right
            anchors.leftMargin: textItem.width
            anchors.verticalCenter: textItem.verticalCenter
        }
        Column{
            id: containerBottom
            z: -10
            spacing: 1
            anchors.top: textItem.bottom
            anchors.topMargin: textItem.height
            anchors.horizontalCenter: textItem.horizontalCenter
        }
        Item{
            id: containerCenter
            z: -10
            anchors.centerIn: textItem
        }
        MouseArea {
            id: mouseArea
            enabled: interactive
            hoverEnabled: enabled
            cursorShape: (enabled && (!drag.active))?Qt.PointingHandCursor:Qt.ArrowCursor
            anchors.fill: textItem
            scale: textItem.scale
            drag.target: missionObject
            onClicked: select()
            onPositionChanged: if(drag.active) objectMoving()
            onDoubleClicked: showMenu()
            onPressAndHold: showMenu()
        }
    }

}
