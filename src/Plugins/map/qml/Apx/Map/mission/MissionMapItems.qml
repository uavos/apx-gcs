import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtQml 2.12

import APX.Vehicles 1.0
import APX.Mission 1.0

MapItemGroup {
    id: missionMapItems
    z: 100
    property Vehicle vehicle: modelData
    property Mission mission: vehicle.mission

    visible: vehicle.active

    Connections {
        target: mission
        onMissionAvailable: {
            //console.log("onMissionReceived")
            map.showRegion(mission.boundingGeoRectangle())
        }
    }



    MapItemView {
        id: waypoints
        model: visible?mission.waypoints.mapModel:0
        delegate: WaypointItem { }
    }

    MapItemView {
        id: runways
        model: visible?mission.runways.mapModel:0
        delegate: RunwayItem { }
    }

    MapItemView {
        id: taxiways
        model: visible?mission.taxiways.mapModel:0
        delegate: TaxiwayItem { }
    }

    MapItemView {
        id: points
        model: visible?mission.points.mapModel:0
        delegate: PointItem { }
    }

    Component.onCompleted: {
        map.addMapItemView(waypoints)
        map.addMapItemView(runways)
        map.addMapItemView(taxiways)
        map.addMapItemView(points)
    }

}
