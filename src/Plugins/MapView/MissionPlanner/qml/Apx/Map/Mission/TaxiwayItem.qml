import QtQuick 2.12
import QtLocation 5.13
import QtGraphicalEffects 1.0

import Apx.Map.Common 1.0

MissionObject {
    id: taxiwayItem
    color: Style.cTaxiway
    textColor: "white"
    fact: modelData
    implicitZ: 32

    property bool largeDataSet: mission.missionSize>100
    shadow: interacting || active || (!largeDataSet)
    radius: shadow?2:0

    //Fact bindings
    property real f_distance: fact?fact.distance:0
    property bool f_current: (m.twidx.value+1) === num
    property bool f_taxi: m.mode.value === mode_TAXI
    property var path: fact?fact.geoPath:0
    property real f_course: fact?fact.course:0
    property bool f_first: num === 0
    property int num: fact?fact.num:0

    //internal
    property bool showDetails: interacting || active || detailsLevel>15 || (map.metersToPixelsFactor*f_distance)>50

    property color pathColor: f_taxi?(f_current?Style.cLineGreen:"yellow"):Style.cNormal
    property int pathWidth: Math.max(1,((f_taxi&&f_current)?6:3)*ui.scale)

    contentsCenter: [
        //courese arrow
        Loader {
            active: (!f_first) && (interacting || showDetails)
            asynchronous: true
            sourceComponent: Component {
                FastBlur {
                    id: crsArrow
                    x: -width/2
                    y: height
                    width: 24
                    height: width
                    transform: Rotation {
                        origin.x: crsArrow.width/2
                        origin.y: -crsArrow.height
                        axis.z: 1
                        angle: taxiwayItem.f_course-map.bearing
                    }
                    radius: ui.antialiasing?4:0
                    opacity: ui.effects?0.6:1
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
            }
        }
    ]



    //Txi Path
    Loader {
        asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapPolyline {
                id: polyline
                z: 10 //f_taxi?taxiwayItem.implicitZ-1:-1
                opacity: ui.effects?0.8:1
                line.width: taxiwayItem.pathWidth
                line.color: taxiwayItem.pathColor
                visible: showDetails && taxiwayItem.visible && (!f_first)
                function updatePath()
                {
                    if(taxiwayItem.path){
                        polyline.setPath(taxiwayItem.path)
                    }
                }
                Connections {
                    target: taxiwayItem
                    onPathChanged: updatePath()
                }
                Component.onCompleted: updatePath()
            }
        }
    }
}
