import QtQuick 2.12
import QtLocation 5.13
import QtGraphicalEffects 1.0

import Apx.Map.Common 1.0

import APX.Mission 1.0 as APX


MissionObject {
    id: waypointItem
    color: visibleOnMap?Style.cWaypoint:"yellow"
    textColor: "black"
    fact: modelData
    implicitZ: 50


    property bool largeDataSet: mission.missionSize>100
    shadow: interacting || active || (!largeDataSet)
    radius: shadow?2:0

    //Fact bindings
    property real f_distance: fact?fact.distance:0
    property real f_totalDistance: fact?fact.totalDistance:0
    property int f_time: fact?fact.time:0
    property int f_totalTime: fact?fact.totalTime:0
    property string actionsText: fact?fact.actions.text:""
    property real f_course: fact?fact.course:0
    property bool f_first: num === 0
    property bool f_warning: fact?fact.warning:false
    property bool f_reachable: fact?fact.reachable:false
    property int f_type: fact?fact.type.value:0
    property var path: fact?fact.geoPath:0
    property int num: fact?fact.num:0


    //internal
    property color cReachable: f_warning?Style.cLineYellow:f_type===APX.Waypoint.Hdg?Style.cLineCyan:Style.cLineBlue
    property color pathColor: f_first?Style.cLineGreen: f_reachable?cReachable:Style.cLineRed
    property real pathWidth: Math.max(1,(f_first?2:4)*ui.scale)


    property bool showDetails: interacting || active || f_distance===0 || (map.metersToPixelsFactor*f_distance)>150

    //property bool pathVisibleOnMap: true
    //property Item pathItem
    //visible: mission.visible && (visibleOnMap || pathVisibleOnMap)
    onUpdateMapViewport: {
        //console.log(typeof(path))
        //if(fact && path)pathVisibleOnMap=map.visibleRegion.boundingGeoRectangle().intersects(path.boundingGeoRectangle())
    }

    contentsTop: [
        Loader {
            active: dragging||hover
            asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cNormal
                    text: apx.distanceToString(f_distance)+"/"+apx.distanceToString(f_totalDistance)
                    font.pixelSize: map.fontSize
                    font.family: font_mono
                    font.bold: false
                }
            }
        },
        Loader {
            active: dragging||hover
            asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cNormal
                    text: (f_time>=120?apx.timeToString(f_time)+"/":"")+apx.timeToString(f_totalTime)
                    font.pixelSize: map.fontSize
                    font.family: font_mono
                    font.bold: false
                }
            }
        }
    ]
    contentsRight: [
        Loader {
            active: (!dragging) && (hover||selected)?1:0
            asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cGreen
                    text: f_altitude.toFixed()+"m"
                    font.pixelSize: map.fontSize
                }
            }
        }
    ]
    contentsBottom: [
        Loader {
            active: actionsText.length>0 && ((!dragging)?((hover||selected)?1:((detailsLevel>13 && showDetails)?(ui.effects?0.6:1):0)):0)
            asynchronous: true
            sourceComponent: Component {
                MapText {
                    textColor: "white"
                    color: Style.cRed
                    text: actionsText
                    font.pixelSize: map.fontSize * 0.8
                    font.bold: false
                }
            }
        }
    ]

    contentsCenter: [
        //courese arrow
        Loader {
            active: waypointItem.active || interacting //&& showDetails
            asynchronous: true
            sourceComponent: Component {
                FastBlur {
                    id: crsArrow
                    x: -width/2
                    y: height
                    z: -1
                    width: 24
                    height: width
                    cached: true
                    transform: Rotation {
                        origin.x: crsArrow.width/2
                        origin.y: -crsArrow.width
                        axis.z: 1
                        angle: waypointItem.f_course-map.bearing
                    }
                    radius: ui.antialiasing?1:0
                    opacity: ui.effects?0.6:1
                    source: ColorOverlay {
                        width: crsArrow.height
                        height: width
                        //smooth: ui.antialiasing
                        source: Image {
                            width: crsArrow.height
                            height: width
                            //smooth: ui.antialiasing
                            source: "../icons/waypoint-course.svg"
                        }
                        color: pathColor
                    }
                }
            }
        }
    ]

    //Flight Path
    Loader {
        asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapPolyline {
                id: polyline
                z: 50 //-90 //waypointItem.implicitZ-1
                visible: waypointItem.visible
                opacity: ui.effects?0.6:1
                //smooth: ui.antialiasing
                line.width: waypointItem.pathWidth
                line.color: waypointItem.pathColor
                function updatePath()
                {
                    if(waypointItem.path){
                        polyline.setPath(waypointItem.path)
                    }
                }
                Connections {
                    target: waypointItem
                    onPathChanged: updatePath()
                }
                Component.onCompleted: updatePath()
            }
        }
    }


}
