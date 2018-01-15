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
    title: (approach>0?"":"H")+fact.title
    fact: modelData
    z: map.z+60

    //Fact bindings
    property int type: fact.type.value
    property string typeText: fact.type.text
    property int approach: fact.approach.value
    property int hmsl: fact.hmsl.value
    property real heading: fact.heading
    property variant endPointCoordinate: fact.endPoint
    property variant appPointCoordinate: fact.appPoint

    property bool current: (m.rwidx.value+1).toFixed() === fact.title
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
            text: typeText.slice(0,1)+Math.floor(app.angle360(heading)).toFixed()
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
    property bool showDetailsRw: map.zoomLevel>15
    property bool showDetailsApp: map.zoomLevel>10
    property real appOpacity: landing?1:0.6

    property bool appHover: hover||appPoint.hover||dragging||appPoint.dragging
    property bool appCircleActive: landing||appHover
    property bool appCircleVisible: showDetailsApp && (approach>0||appHover)
    property int appCircleLineWidth: appCircleActive?2:1
    property real appCircleOpacity: appCircleActive?0.8:0.6

    property variant appCircleAppCoord: appPointCoordinate
    property variant appCircleCoordinate: appCircleAppCoord.atDistanceAndAzimuth(appCircleRadius,heading+(type===Runway.Left?-90:90))
    property real appCircleRadius: Math.max(100,landing?turnR:approach/2)

    Component.onCompleted: {
        var c=endPointC.createObject(map)
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
        c=appPointC.createObject(map)
        appPoint=c
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
        //runway path
        c=pathC.createObject(map)
        c.z=map.z+10
        c.line.width=10
        c.line.color=Qt.binding(function(){return cRwBG})
        c.visible=Qt.binding(function(){return current})
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
        c=pathC.createObject(map)
        c.z=map.z+11
        c.line.width=4
        c.line.color="white"
        c.opacity=0.8
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
        c=pathC.createObject(map)
        c.z=map.z+12
        c.line.width=2
        c.line.color="black"
        c.opacity=0.5
        c.visible=Qt.binding(function(){return showDetailsRw})
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
        //approach path
        c=pathC.createObject(map)
        c.z=map.z+1
        c.line.width=1
        c.line.color="white"
        c.opacity=Qt.binding(function(){return appOpacity})
        c.visible=Qt.binding(function(){return showDetailsApp})
        c.coordinate2=Qt.binding(function(){return appPointCoordinate})
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
        //approach circle
        c=circleC.createObject(map)
        c.z=map.z+1
        c.border.color="white"
        c.border.width=Qt.binding(function(){return appCircleLineWidth})
        c.opacity=Qt.binding(function(){return appCircleOpacity})
        c.visible=Qt.binding(function(){return appCircleVisible})
        c.center=Qt.binding(function(){return appCircleCoordinate})
        c.radius=Qt.binding(function(){return appCircleRadius})
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})


    }
    Component {
        id: endPointC
        MissionObject {
            id: endPoint
            z: map.z+40
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
            color: "white"
            textColor: "black"
            title: approach>0?(app.distanceToString(approach)):"H----"
            property double r: runwayItem.heading-90;
            rotation: approach>0?app.angle90(r):app.angle(r);
            visible: showDetailsApp
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
        MapPolyline {
            id: polyline
            property variant coordinate1: runwayItem.coordinate
            property variant coordinate2: endPointCoordinate
            function updatePath()
            {
                polyline.replaceCoordinate(0,coordinate1)
                polyline.replaceCoordinate(1,coordinate2)
            }
            onCoordinate1Changed: updatePath()
            onCoordinate2Changed: updatePath()
            Component.onCompleted: {
                polyline.addCoordinate(coordinate1)
                polyline.addCoordinate(coordinate2)
                updatePath()
            }
        }
    }
    Component {
        id: circleC
        MapCircle {
            id: circle
            color: "transparent"
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
