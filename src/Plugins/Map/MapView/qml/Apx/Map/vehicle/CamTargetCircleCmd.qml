import QtQuick 2.5
import QtPositioning 5.6

import "../lib"

MapIcon {

    name: "blur"

    //Fact bindings
    property real lat: m.cam_lat.value
    property real lon: m.cam_lon.value
    property real hmsl: m.cam_hmsl.value

    coordinate: QtPositioning.coordinate(lat,lon,hmsl)
}
