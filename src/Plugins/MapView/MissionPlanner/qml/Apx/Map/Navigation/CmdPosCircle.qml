import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapCircle {
    color: "#1000FF00"
    border.color: "magenta"
    border.width: 1

    //Fact bindings
    readonly property real cmd_lat: mandala.cmd.pos.lat.value
    readonly property real cmd_lon: mandala.cmd.pos.lon.value

    property real turnR: mandala.cmd.proc.radius.value
    property bool landing: mandala.cmd.proc.mode.value === proc_mode_LANDING

    center: QtPositioning.coordinate(cmd_lat,cmd_lon)
    radius: Math.max(landing?turnR:0,50)
}
