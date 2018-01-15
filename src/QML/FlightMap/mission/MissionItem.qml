import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.FactSystem 1.0
import GCS.Vehicles 1.0
import "."

MapItemGroup {
    id: missionItem
    property Vehicle vehicle: modelData

    MapItemView {
        id: waypoints
        model: vehicle.mission.waypoints.model
        delegate: WaypointItem { }
    }

    MapItemView {
        id: runways
        model: vehicle.mission.runways.model
        delegate: RunwayItem { }
    }

    MapItemView {
        id: taxiways
        model: vehicle.mission.taxiways.model
        delegate: TaxiwayItem { }
    }

    MapItemView {
        id: points
        model: vehicle.mission.points.model
        delegate: PointItem { }
    }

    Component.onCompleted: {
        map.addMapItemView(waypoints)
        map.addMapItemView(runways)
        map.addMapItemView(taxiways)
        map.addMapItemView(points)
    }

}
