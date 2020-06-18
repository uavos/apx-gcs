import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapCircle {
    color: "#1000FF00"
    border.color: "magenta"
    border.width: 1

    //Fact bindings
    property real cmd_lat: mandala.cmd.pos.lat.value
    property real cmd_lon: mandala.cmd.pos.lon.value

    property real turnR: mandala.est.ctr.radius.value
    property bool landing: mandala.cmd.op.mode.value === op_mode_LANDING

    center: QtPositioning.coordinate(cmd_lat,cmd_lon)
    radius: Math.max(landing?turnR:0,50)
}
