import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import ".."

MapPolyline {
    z: 50
    opacity: ui.effects?0.8:1
    line.width: 2
    line.color: vehicle.telemetry.active?Style.cLineGreen:Style.cBlue

    property var vehicle: apx.vehicles.current

    onVehicleChanged: updatePath()

    Connections {
        target: vehicle
        onGeoPathAppend: {
            addCoordinate(p)
        }
    }
    Connections {
        target: vehicle.action.rpath
        onTriggered: {
            setPath(QtPositioning.path())
        }
    }

    function updatePath()
    {
        setPath(vehicle.geoPath)
    }

    Component.onCompleted: updatePath()
}
