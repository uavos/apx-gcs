import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapQuickItem {

    //Fact bindings
    property real cmd_lat: mandala.cmd.pos.lat.value
    property real cmd_lon: mandala.cmd.pos.lon.value

    coordinate: QtPositioning.coordinate(cmd_lat,cmd_lon)

    //constants
    anchorPoint.x: icon.implicitWidth/2
    anchorPoint.y: icon.implicitHeight*0.85

    sourceItem:
    MaterialIcon {
        id: icon
        size: 48
        name: "chevron-double-down"
        opacity: ui.effects?0.5:1
    }
}
