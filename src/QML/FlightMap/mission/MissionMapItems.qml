import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.FactSystem 1.0
import GCS.Vehicles 1.0
import GCS.Mission 1.0
import "."

MapItemGroup {
    id: missionMapItems
    property Vehicle vehicle: modelData
    property Mission mission: vehicle.mission

    visible: vehicle.active

    function select(obj)
    {
        if(map.selectedObject!==obj) map.selectedObject=obj
    }
    function deselect()
    {
        if(map.selectedObject) map.selectedObject=null
    }

    function showMapMenu()
    {
        deselect()

    }

    Connections {
        target: map
        onClicked: deselect()
        onMapMenuRequested: showMapMenu()
        onMouseClickCoordinateChanged: mission.mapCoordinate=map.mouseClickCoordinate
    }

    Connections {
        target: mission
        onMissionReceived: {
            //console.log("onMissionReceived")
            map.showRegion(mission.boundingGeoRectangle())
        }
    }




    MapItemView {
        id: waypoints
        model: visible?mission.waypoints.model:0
        delegate: WaypointItem { }
    }

    MapItemView {
        id: runways
        model: visible?mission.runways.model:0
        delegate: RunwayItem { }
    }

    MapItemView {
        id: taxiways
        model: visible?mission.taxiways.model:0
        delegate: TaxiwayItem { }
    }

    MapItemView {
        id: points
        model: visible?mission.points.model:0
        delegate: PointItem { }
    }

    Component.onCompleted: {
        map.addMapItemView(waypoints)
        map.addMapItemView(runways)
        map.addMapItemView(taxiways)
        map.addMapItemView(points)
    }

}
