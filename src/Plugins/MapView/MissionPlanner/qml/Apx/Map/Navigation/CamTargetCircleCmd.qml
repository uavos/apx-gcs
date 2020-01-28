import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Map.Common 1.0

MapIcon {

    name: "blur"

    //Fact bindings
    property real lat: mandala.est.cam.lat.value
    property real lon: mandala.est.cam.lon.value
    property real hmsl: mandala.est.cam.hmsl.value

    coordinate: QtPositioning.coordinate(lat,lon,hmsl)
}
