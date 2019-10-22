import QtQuick 2.12
import QtLocation 5.13

import APX.Vehicles 1.0 as APX
import APX.Mission 1.0 as APX


MapItemGroup {
    id: group
    z: 100

    Component.onCompleted: {
        map.addMapItemGroup(group)
    }

    property APX.Vehicle vehicle: apx.vehicles.current
    property APX.Mission mission: vehicle.mission

    visible: !mission.empty

    Connections {
        enabled: !map.follow
        target: mission
        onMissionAvailable: showRegion()
    }

    function showRegion()
    {
        //if(mission.empty) return
        var r=mission.boundingGeoRectangle().united(vehicle.geoPathRect())

        if(!map.visibleRegion.boundingGeoRectangle().intersects(r))
            map.showRegion(r)
    }



    MapItemView {
        z: 50
        model: mission.waypoints.mapModel
        delegate: WaypointItem { }
    }

    MapItemView {
        z: 60
        model: mission.runways.mapModel
        delegate: RunwayItem { }
    }

    MapItemView {
        z: 0
        model: mission.taxiways.mapModel
        delegate: TaxiwayItem { }
    }

    MapItemView {
        z: 20
        model: mission.points.mapModel
        delegate: PointItem { }
    }
}
