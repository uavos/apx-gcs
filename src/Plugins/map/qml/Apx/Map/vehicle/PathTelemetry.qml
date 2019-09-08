import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import ".."

MapPolyline {
    z: 50
    opacity: ui.effects?0.6:1
    line.width: 4
    line.color: Style.cBlue
    path: apx.vehicles.current.telemetry.reader.geoPath

    property var p: apx.vehicles.current.telemetry.reader.geoPath
    function updatePath()
    {
        setPath(p)
        showRegion()
    }
    function showRegion()
    {
        if(path.length>0){
            map.showRegion(apx.vehicles.current.telemetry.reader.geoRect)
        }
    }

    Connections {
        target: apx.vehicles.current.telemetry.reader
        onGeoPathChanged: updatePath()
        onTriggered: showRegion()
    }
    Component.onCompleted: updatePath()
}
