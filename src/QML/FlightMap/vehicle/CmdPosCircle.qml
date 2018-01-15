import QtQuick 2.5
import QtLocation 5.6
import QtPositioning 5.6
import "../helper.js" as Helper

MapCircle {
    color: "#1000FF00"
    border.color: "magenta"
    border.width: 1
    smooth: true
    //opacity: 0.9

    //Fact bindings
    property real cmd_east: m.cmd_east.value
    property real cmd_north: m.cmd_north.value
    property real home_lat: m.home_lat.value
    property real home_lon: m.home_lon.value

    property real turnR: m.turnR.value
    property bool landing: m.mode.value === mode_LANDING

    //calc coordinate
    property variant homeCoord: QtPositioning.coordinate(home_lat,home_lon)
    property real azimuth: Math.degrees(Math.atan2(cmd_east,cmd_north))
    property real distance: Math.sqrt(Math.pow(cmd_east,2)+Math.pow(cmd_north,2))

    center: homeCoord.atDistanceAndAzimuth(distance,azimuth)
    radius: Math.max(landing?turnR:0,50)
}
