import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapCircle {
    color: "#100000FF"
    border.color: "#500000FF"
    border.width: 2

    //Fact bindings
    property real cmd_east: m.cmd_east.value
    property real cmd_north: m.cmd_north.value
    property real home_lat: m.home_lat.value
    property real home_lon: m.home_lon.value
    property real turnR: m.turnR.value
    property real mode: m.mode.value

    //calc coordinate
    property variant homeCoord: QtPositioning.coordinate(home_lat,home_lon)
    property real azimuth: Apx.degrees(Math.atan2(cmd_east,cmd_north))
    property real distance: Math.sqrt(Math.pow(cmd_east,2)+Math.pow(cmd_north,2))

    center: homeCoord.atDistanceAndAzimuth(distance,azimuth)
    radius: Math.abs(turnR)
    visible: mode===mode_STBY

    Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
}
