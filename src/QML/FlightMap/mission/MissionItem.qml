import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.FactSystem 1.0
import GCS.Vehicles 1.0
import "."

MapItemGroup {
    id: mission
    property Vehicle vehicle: modelData

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
        onMouseClickCoordinateChanged: vehicle.mission.mapCoordinate=map.mouseClickCoordinate
    }




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
