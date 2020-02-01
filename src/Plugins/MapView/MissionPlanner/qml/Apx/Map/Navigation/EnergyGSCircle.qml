import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

MapCircle {
    color: "transparent"
    border.color: "#8000FFFF"
    border.width: 10
    //smooth: ui.antialiasing
    radius: 50

    //Fact bindings
    property real lat: mandala.est.pos.lat.value
    property real lon: mandala.est.pos.lon.value
    property real altitude: mandala.est.air.altitude.value
    property real gSpeed: mandala.est.pos.speed.value
    property real course: mandala.est.pos.course.value
    property real gps_Vdown: mandala.est.rel.vd.value

    //calculate Energy Circle based on Ground Speed and descending rate
    property bool gPerfOk: gSpeed>0.5 && gps_Vdown>0.5
    property real gPerf: 0
    property real distance: altitude*gPerf
    center: QtPositioning.coordinate(lat,lon).atDistanceAndAzimuth(distance,course)

    /*onGSpeedChanged: {
        if(gSpeed>0.5 && gps_Vdown>0.5){
            gPerf=gPerf*0.9+(gSpeed/gps_Vdown)*0.1
        }
    }

    onAltitudeChanged: {
        if(gSpeed>0.5 && gps_Vdown>0.5){
            gPerf=gSpeed/gps_Vdown
            distance=altitude
        }
    }*/
    //visual
    //visible: altitude>5 && gSpeed>1
    //Behavior on distance { enabled: ui.smooth; NumberAnimation {duration: 5000;} }
}
