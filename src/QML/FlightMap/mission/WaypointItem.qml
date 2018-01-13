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

    //Fact bindings
    property real wDW: fact.DW
    property real wDT: fact.DT
    property int wTime: fact.time
    property int wETA: fact.ETA
    property string actionsText: "some"
    property real wCourse: fact.course
    property bool bFirst: fact.title==='1'
    property bool warning: fact.warning
    property bool reachable: fact.reachable
    property int type: fact.type.value
    property var path: fact.flightPath


    //internal
    property color cReachable: warning?Style.cLineYellow:type===Waypoint.Hdg?Style.cLineCyan:Style.cLineBlue
    property color pathColor: bFirst?Style.cLineGreen: reachable?cReachable:Style.cLineRed
    property int pathWidth: bFirst?2:4


    property bool showDetails: wDW===0 || map.metersToPixels(wDW)>50

    contentsTop: [
        MapText {
            textColor: "white"
            color: Style.cNormal
            text: app.distanceToString(wDW)+"/"+app.distanceToString(wDT) //+"/"+path.size()
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
        },
        MapText {
            textColor: "white"
            color: Style.cNormal
            text: app.timeToString(wTime)+"/"+app.timeToString(wETA)
            opacity: (dragging||hover)?1:0
            visible: opacity
            font.family: font_mono
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
            visible: actionsText!=="off" && opacity>0
            textColor: "white"
            color: Style.cRed
            text: actionsText
            opacity: (!dragging)?((hover||selected)?1:((map.zoomLevel>13 && showDetails)?0.6:0)):0
        }
    ]

    contentsCenter: [
        FastBlur {
            id: crsArrow
            x: -width/2
            y: height
            width: 24
            height: width
            transform: Rotation {
                origin.x: crsArrow.width/2
                origin.y: -crsArrow.width
                axis.z: 1
                angle: wCourse-map.bearing
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
        var c=pathC.createObject(map,{"wp": fact})
        map.addMapItem(c)
        fact.removed.connect(function(){c.destroy()})
    }
    Component {
        id: pathC
        MapPolyline {
            id: polyline
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
