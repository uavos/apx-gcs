import QtQuick 2.5
import QtLocation 5.6
import QtPositioning 5.6
import "../helper.js" as Helper

MapCircle {
    color: "#100000FF"
    border.color: "#500000FF"
    border.width: 2
    smooth: true
    //opacity: 0.9

    //Fact bindings
    property real cmd_east: m.cmd_east.value
    property real cmd_north: m.cmd_north.value
    property real home_lat: m.home_lat.value
    property real home_lon: m.home_lon.value
    property real turnR: m.turnR.value
    property real mode: m.mode.value

    //calc coordinate
    property variant homeCoord: QtPositioning.coordinate(home_lat,home_lon)
    property real azimuth: Math.degrees(Math.atan2(cmd_east,cmd_north))
    property real distance: Math.sqrt(Math.pow(cmd_east,2)+Math.pow(cmd_north,2))

    center: homeCoord.atDistanceAndAzimuth(distance,azimuth)
    radius: Math.abs(turnR)
    visible: mode===mode_STBY

    Behavior on radius { enabled: app.settings.smooth.value; NumberAnimation {duration: 100;} }
}
