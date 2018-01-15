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
    color: Style.cWaypoint
    textColor: "black"
    fact: modelData
    z: map.z+50

    //Fact bindings
    property real distance: fact.distance
    property real totalDistance: fact.totalDistance
    property int time: fact.time
    property int totalTime: fact.totalTime
    property string actionsText: fact.actions.status
    property real course: fact.course
    property bool bFirst: fact.title==='1'
    property bool warning: fact.warning
    property bool reachable: fact.reachable
    property int type: fact.type.value
    property var path: fact.travelPath


    //internal
    property color cReachable: warning?Style.cLineYellow:type===Waypoint.Hdg?Style.cLineCyan:Style.cLineBlue
    property color pathColor: bFirst?Style.cLineGreen: reachable?cReachable:Style.cLineRed
    property int pathWidth: bFirst?2:4


    property bool showDetails: distance===0 || map.metersToPixels(distance)>50

    contentsTop: [
        MapText {
            textColor: "white"
            color: Style.cNormal
            text: app.distanceToString(distance)+"/"+app.distanceToString(totalDistance) //+"/"+path.size()
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
            font.bold: false
        },
        MapText {
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
            textColor: "white"
            color: Style.cGreen
            text: altitude.toFixed()+"m"
            opacity: (!dragging) && (hover||selected)?1:0
            visible: opacity
        }
    ]
    contentsBottom: [
        MapText {
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
            z: map.z
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
        var c=pathC.createObject(map)
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
    }
    Component {
        id: pathC
        MapPolyline {
            id: polyline
            z: map.z
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
