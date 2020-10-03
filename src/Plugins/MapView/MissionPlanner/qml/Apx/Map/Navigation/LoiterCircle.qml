import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapCircle {
    color: "#100000FF"
    border.color: "#500000FF"
    border.width: 2

    //Fact bindings
    property real cmd_lat: mandala.cmd.pos.lat.value
    property real cmd_lon: mandala.cmd.pos.lon.value

    property real turnR: mandala.cmd.pos.radius.value
    property int mode: mandala.cmd.reg.pos.value

    center: QtPositioning.coordinate(cmd_lat,cmd_lon)
    radius: Math.abs(turnR)
    visible: mode===reg_pos_loiter

    Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
}
