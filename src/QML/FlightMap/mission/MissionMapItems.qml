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
        model: mission.waypoints.model
        delegate: WaypointItem { fact: modelData }
    }

    MapItemView {
        id: runways
        model: mission.runways.model
        delegate: RunwayItem { }
    }

    MapItemView {
        id: taxiways
        model: mission.taxiways.model
        delegate: TaxiwayItem { fact: modelData }
    }

    MapItemView {
        id: points
        model: mission.points.model
        delegate: PointItem { }
    }

    Component.onCompleted: {
        map.addMapItemView(waypoints)
        map.addMapItemView(runways)
        map.addMapItemView(taxiways)
        map.addMapItemView(points)
    }

}
