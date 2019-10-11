import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Map.Common 1.0

MapIcon {

    name: "blur"

    //Fact bindings
    property real lat: m.cam_lat.value
    property real lon: m.cam_lon.value
    property real hmsl: m.cam_hmsl.value

    coordinate: QtPositioning.coordinate(lat,lon,hmsl)
}
