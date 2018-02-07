import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import GCS.Mission 1.0
import QtGraphicalEffects 1.0
import "."
import ".."


MissionObject {
    id: waypointItem
    color: visibleOnMap?Style.cWaypoint:"yellow"
    textColor: "black"
    fact: modelData
    implicitZ: 50

    //Fact bindings
    property real distance: fact.distance
    property real totalDistance: fact.totalDistance
    property int time: fact.time
    property int totalTime: fact.totalTime
    property string actionsText: fact.actions.status
    property real course: fact.course
    property bool bFirst: fact.num === 0
    property bool warning: fact.warning
    property bool reachable: fact.reachable
    property int type: fact.type.value
    property var path: fact.geoPath


    //internal
    property color cReachable: warning?Style.cLineYellow:type===Waypoint.Hdg?Style.cLineCyan:Style.cLineBlue
    property color pathColor: bFirst?Style.cLineGreen: reachable?cReachable:Style.cLineRed
    property int pathWidth: bFirst?2:4


    property bool showDetails: distance===0 || map.metersToPixels(distance)>50

    property bool pathVisibleOnMap: true
    property Item pathItem
    visible: mission.visible && (visibleOnMap || pathVisibleOnMap)
    onUpdateMapViewport: {
        pathVisibleOnMap=map.visibleRegion.boundingGeoRectangle().intersects(path.boundingGeoRectangle())
    }

    contentsTop: [
        MapText {
            z: 10
            textColor: "white"
            color: Style.cNormal
            text: app.distanceToString(distance)+"/"+app.distanceToString(totalDistance) //+"/"+path.size()
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
            font.bold: false
        },
        MapText {
            z: 10
            textColor: "white"
            color: Style.cNormal
            text: app.timeToString(time)+"/"+app.timeToString(totalTime)
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
            font.bold: false
        }
    ]
    contentsRight: [
        MapText {
            z: 10
            textColor: "white"
            color: Style.cGreen
            text: altitude.toFixed()+"m"
            opacity: (!dragging) && (hover||selected)?1:0
            visible: opacity
        }
    ]
    contentsBottom: [
        MapText {
            z: 10
            visible: actionsText.length>0 && opacity>0
            textColor: "white"
            color: Style.cRed
            text: actionsText
            opacity: (!dragging)?((hover||selected)?1:((map.zoomLevel>13 && showDetails)?0.6:0)):0
            font.pixelSize: Qt.application.font.pixelSize * 0.8
            font.bold: false
        }
    ]

    contentsCenter: [
        //courese arrow
        FastBlur {
            id: crsArrow
            x: -width/2
            y: height
            z: -1
            width: 24
            height: width
            transform: Rotation {
                origin.x: crsArrow.width/2
                origin.y: -crsArrow.width
                axis.z: 1
                angle: course-map.bearing
            }
            radius: 4
            opacity: 0.6
            visible: showDetails
            source: ColorOverlay {
                width: crsArrow.height
                height: width
                source: Image {
                    width: crsArrow.height
                    height: width
                    source: "../icons/waypoint-course.svg"
                }
                color: pathColor
            }
        }
    ]

    //Flight Path
    Component.onCompleted: {
        pathItem=createMapComponent(pathC)
    }
    Component {
        id: pathC
        MapPolyline {
            id: polyline
            z: map.z
            visible: waypointItem.visible
            opacity: 0.6
            line.width: waypointItem.pathWidth
            line.color: waypointItem.pathColor
            function updatePath()
            {
                polyline.setPath(waypointItem.path)
            }
            Connections {
                target: waypointItem
                onPathChanged: updatePath()
            }
            Component.onCompleted: updatePath()
        }
    }

}
