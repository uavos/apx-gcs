import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import GCS.Mission 1.0
import QtGraphicalEffects 1.0
import "."
import ".."


MissionObject {
    id: taxiwayItem
    color: Style.cTaxiway
    textColor: "white"
    fact: modelData
    implicitZ: 30

    //Fact bindings
    property real distance: fact.distance
    property bool current: m.twidx.value === fact.num
    property bool taxi: m.mode.value === mode_TAXI
    property var path: fact.geoPath
    property real course: fact.course
    property bool bFirst: fact.num === 0

    //internal
    property bool showDetails: map.zoomLevel>15 ||  map.metersToPixels(distance)>50

    property color pathColor: taxi?(current?Style.cLineGreen:"yellow"):Style.cNormal
    property int pathWidth: (taxi&&current)?6:3

    contentsCenter: [
        //courese arrow
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
                angle: course-map.bearing
            }
            radius: 4
            opacity: 0.6
            visible: showDetails && (!bFirst)
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



    //Txi Path
    Component.onCompleted: {
        createMapComponent(pathC)
    }
    Component {
        id: pathC
        MapPolyline {
            id: polyline
            z: map.z+(taxi?30:0)
            opacity: 0.8
            line.width: taxiwayItem.pathWidth
            line.color: taxiwayItem.pathColor
            visible: showDetails && taxiwayItem.visible && (!bFirst)
            function updatePath()
            {
                polyline.setPath(taxiwayItem.path)
            }
            Connections {
                target: taxiwayItem
                onPathChanged: updatePath()
            }
            Component.onCompleted: updatePath()
            //Component.onDestruction: console.log(this)
        }
    }
}
