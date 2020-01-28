import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapCircle {
    color: "#100000FF"
    border.color: "#500000FF"
    border.width: 2

    //Fact bindings
    property real cmd_east: mandala.cmd.pos.e.value
    property real cmd_north: mandala.cmd.pos.n.value
    property real home_lat: mandala.est.ref.lat.value
    property real home_lon: mandala.est.ref.lon.value
    property real turnR: mandala.est.ctr.radius.value
    property int mode: mandala.cmd.op.mode.value

    //calc coordinate
    property variant homeCoord: QtPositioning.coordinate(home_lat,home_lon)
    property real azimuth: Apx.degrees(Math.atan2(cmd_east,cmd_north))
    property real distance: Math.sqrt(Math.pow(cmd_east,2)+Math.pow(cmd_north,2))

    center: homeCoord.atDistanceAndAzimuth(distance,azimuth)
    radius: Math.abs(turnR)
    visible: mode===op_mode_STBY

    Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
}
