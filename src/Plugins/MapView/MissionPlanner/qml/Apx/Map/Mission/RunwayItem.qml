import QtQuick 2.12
import QtLocation 5.13
import QtGraphicalEffects 1.0

import Apx.Map.Common 1.0

import APX.Mission 1.0 as APX


MissionObject {
    id: runwayItem
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
    property var endPointCoordinate: fact && fact.endPoint?fact.endPoint:coordinate
    property var appPointCoordinate: fact && fact.appPoint?fact.appPoint:coordinate
    property int num: fact?fact.num:0

    property bool f_current: m.rwidx.value === num
    property bool f_landing: m.mode.value === mode_LANDING && f_current
    property real f_turnR: m.turnR.value
    property real f_delta: m.delta.value

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
            asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: f_current?Style.cBlue:Style.cNormal
                    text: rwName
                }
            }
        },
        Loader {
            active: showDetails && f_hmsl!=0 && ((!dragging)?((hover||selected)?1:(ui.effects?0.6:1)):0)
            asynchronous: true
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
    property color cRwBG: f_landing?"blue":Style.cBlue
    property bool showDetailsRw: runwayItem.visible && detailsLevel>15
    property bool showDetailsApp: runwayItem.visible && detailsLevel>10
    property real appOpacity: ui.effects?(f_landing?1:0.6):1

    property bool appHover: hover||appPoint.hover||dragging||appPoint.dragging
    property bool appCircleActive: f_landing||appHover
    property bool appCircleVisible: runwayItem.visible && showDetailsApp && (f_approach>0||appHover)
    property int appCircleLineWidth: Math.max(1,(appCircleActive?2:1)*ui.scale)
    property real appCircleOpacity: ui.effects?(appCircleActive?0.8:0.6):1

    property variant appCircleAppCoord: appPointCoordinate
    property variant appCircleCoordinate: appCircleAppCoord.atDistanceAndAzimuth(appCircleRadius,f_heading+(f_type===APX.Runway.Left?-90:90))
    property real appCircleRadiusDefault: f_approach/2
    property real appCircleRadius: Math.max(100,f_landing?Math.abs(f_turnR):appCircleRadiusDefault)


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
        c.visible=Qt.binding(function(){return f_current && runwayItem.visible})

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
        c.center=Qt.binding(function(){return appCircleCoordinate})
        c.radius=Qt.binding(function(){return appCircleRadius})

        //delta rect
        c=createMapComponent(deltaC)
        c.z=1
    }

    //handles
    Loader {
        //appPoint
        asynchronous: true
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
        asynchronous: true
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
                        asynchronous: true
                        sourceComponent: Component {
                            MapText {
                                textColor: "white"
                                color: f_current?Style.cBlue:Style.cNormal
                                text: runwayItem.rwName + " ("+Math.floor(apx.angle360(f_heading)).toFixed()+")"
                            }
                        }
                    }
                ]
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
            visible: runwayItem.visible && f_landing && v!=0

            property int v: isFinite(f_delta)?f_delta:0
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
