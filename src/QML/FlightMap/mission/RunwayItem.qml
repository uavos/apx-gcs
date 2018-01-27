import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import GCS.Mission 1.0
import QtGraphicalEffects 1.0
import "."
import ".."


MissionObject {
    id: runwayItem
    color: Style.cRunway
    textColor: "white"
    title: (approach>0?"":"H")+(fact.num+1)
    fact: modelData
    implicitZ: 60

    //Fact bindings
    property int type: fact.type.value
    property string typeText: fact.type.text
    property string titleText: fact.title
    property int approach: fact.approach.value
    property int hmsl: fact.hmsl.value
    property real heading: fact.heading
    property variant endPointCoordinate: fact.endPoint
    property variant appPointCoordinate: fact.appPoint

    property bool current: m.rwidx.value === fact.num
    property bool landing: m.mode.value === mode_LANDING && current
    property real turnR: m.turnR.value
    property real delta: m.delta.value

    function updateEndPoint(coord)
    {
        fact.endPoint=coord
    }
    function updateAppPoint(coord)
    {
        fact.appPoint=coord
    }

    //internal
    property bool showDetails: map.zoomLevel>13


    contentsRight: [
        MapText {
            textColor: "white"
            color: current?Style.cBlue:Style.cNormal
            text: titleText.split(' ')[1]
            opacity: (!dragging)?((hover||selected)?1:0.6):0
            visible: opacity && showDetails
        },
        MapText {
            visible: opacity && showDetails && hmsl!=0
            textColor: "white"
            color: Style.cGreen
            text: hmsl+"m"
            opacity: (!dragging)?((hover||selected)?1:0.6):0
        }
    ]

    contentsCenter: [
        Item {
            //approach delta rectangle
            Rectangle {
                id: deltaRect
                //z: -999
                visible: landing //&& height>10
                property int v: delta
                Behavior on v { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
                anchors.left: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                property bool warn: v<-300
                property real sf: map.metersToPixels(1)
                width: Math.max(5,Math.abs(v*sf))
                height: 45*sf
                color: warn?"#60FF0000":(v>=150||v<-50)?"#40FFFF00":"#4000FF00"
                Behavior on color { enabled: app.settings.smooth.value; ColorAnimation {duration: 200; } }
                border.color: warn?"#E0FF0000":"#C000FF00"
                border.width: 1
                radius: Math.min(5,height*0.1)
                transform: [
                    Rotation {
                        origin.x: 0
                        origin.y: deltaRect.height/2
                        axis.z: 1
                        angle: runwayItem.heading+(deltaRect.v>=0?-90:180-90)
                    }
                ]
            }
        }
    ]



    //Runway Map Items
    property Item appPoint
    property color cRwBG: landing?"blue":Style.cBlue
    property bool showDetailsRw: runwayItem.visible && map.zoomLevel>15
    property bool showDetailsApp: runwayItem.visible && map.zoomLevel>10
    property real appOpacity: landing?1:0.6

    property bool appHover: hover||appPoint.hover||dragging||appPoint.dragging
    property bool appCircleActive: landing||appHover
    property bool appCircleVisible: runwayItem.visible && showDetailsApp && (approach>0||appHover)
    property int appCircleLineWidth: appCircleActive?2:1
    property real appCircleOpacity: appCircleActive?0.8:0.6

    property variant appCircleAppCoord: appPointCoordinate
    property variant appCircleCoordinate: appCircleAppCoord.atDistanceAndAzimuth(appCircleRadius,heading+(type===Runway.Left?-90:90))
    property real appCircleRadiusDefault: approach/2
    property real appCircleRadius: Math.max(100,landing?turnR:appCircleRadiusDefault)
    //Behavior on appCircleRadiusDefault { enabled: app.settings.smooth.value; NumberAnimation {duration: 100;} }

    Component.onCompleted: {
        createMapComponent(endPointC)
        appPoint=createMapComponent(appPointC)
        //runway path
        var c=createMapComponent(pathC)
        c.z=map.z+10
        c.line.width=10
        c.line.color=Qt.binding(function(){return cRwBG})
        c.opacity=0.9
        c.visible=Qt.binding(function(){return current && runwayItem.visible})

        c=createMapComponent(pathC)
        c.z=map.z+11
        c.line.width=4
        c.line.color="white"
        c.opacity=0.8

        c=createMapComponent(pathC)
        c.z=map.z+12
        c.line.width=2
        c.line.color="black"
        c.opacity=0.5
        c.visible=Qt.binding(function(){return showDetailsRw})

        //approach path
        c=createMapComponent(pathC)
        c.z=map.z+1
        c.line.width=Qt.binding(function(){return appCircleLineWidth})
        c.line.color="white"
        c.opacity=Qt.binding(function(){return appOpacity})
        c.visible=Qt.binding(function(){return showDetailsApp})
        c.p2=Qt.binding(function(){return appPointCoordinate})

        //approach circle
        c=createMapComponent(circleC)
        c.z=map.z+1
        c.border.color="white"
        c.border.width=Qt.binding(function(){return appCircleLineWidth})
        c.opacity=Qt.binding(function(){return appCircleOpacity})
        c.visible=Qt.binding(function(){return appCircleVisible})
        c.center=Qt.binding(function(){return appCircleCoordinate})
        c.radius=Qt.binding(function(){return appCircleRadius})
    }
    Component {
        id: endPointC
        MissionObject {
            id: endPoint
            z: map.z+40
            visible: runwayItem.visible
            color: "white"
            textColor: "black"
            opacity: 0.8
            title: runwayItem.title
            factPos: endPointCoordinate
            onMoved: updateEndPoint(coordinate)
            contentsRight: [
                MapText {
                    textColor: "white"
                    color: current?Style.cBlue:Style.cNormal
                    text: Math.floor(app.angle360(heading)).toFixed()
                    opacity: (dragging||hover||selected)?1:0
                    visible: opacity && showDetails
                }
            ]
        }
    }
    Component {
        id: appPointC
        MissionObject {
            id: appPoint
            z: map.z+40-1
            visible: runwayItem.visible && showDetailsApp
            color: "white"
            textColor: "black"
            title: approach>0?(app.distanceToString(approach)):"H----"
            property double r: runwayItem.heading-90;
            rotation: approach>0?app.angle90(r):app.angle(r);
            factPos: appPointCoordinate
            onMoved: {
                updateAppPoint(coordinate)
                coordinate=factPos //snap
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
    /*Component {
        id: deltaRectC
        //approach delta rectangle
        MapPolygon {
            id: deltaRect
            property variant coordinate: runwayItem.coordinate
            visible: landing
            property int v: delta
            Behavior on v { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
            property bool warn: v<-300
            //width: v //Math.max(5,Math.abs(v*sf))
            //height: 45*sf
            color: warn?"#60FF0000":(v>=150||v<-50)?"#40FFFF00":"#4000FF00"
            border.color: warn?"#E0FF0000":"#C000FF00"
            border.width: 2
            //radius: Math.min(5,height*0.1)
            transform: Rotation {
                origin.x: 0
                origin.y: deltaRect.height/2
                axis.z: 1
                angle: runwayItem.mItem.heading+(deltaRect.v>=0?-90:180-90)
            }
            Behavior on color { enabled: app.settings.smooth.value; ColorAnimation {duration: 200; } }
        }

    }*/

}
