import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtQml 2.12

import APX.Vehicles 1.0
import APX.Mission 1.0
import ".."
import "../lib"


MapItemGroup {
    id: missionMapItems
    z: 100
    property Vehicle vehicle: modelData
    property Mission mission: vehicle.mission

    visible: vehicle.active

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



    Loader {
        active: parent.visible
        asynchronous: true
        onLoaded: map.addMapItemView(item)
        sourceComponent: Component {
            MapItemView {
                z: 150
                model: mission.waypoints.mapModel
                delegate: WaypointItem { }
            }
        }
    }
    Loader {
        active: parent.visible
        asynchronous: true
        onLoaded: map.addMapItemView(item)
        sourceComponent: Component {
            MapItemView {
                z: 151
                model: mission.runways.mapModel
                delegate: RunwayItem { }
            }
        }
    }
    Loader {
        active: parent.visible
        asynchronous: true
        onLoaded: map.addMapItemView(item)
        sourceComponent: Component {
            MapItemView {
                z: 100
                model: mission.taxiways.mapModel
                delegate: TaxiwayItem { }
            }
        }
    }
    Loader {
        active: parent.visible
        asynchronous: true
        onLoaded: map.addMapItemView(item)
        sourceComponent: Component {
            MapItemView {
                z: 120
                model: mission.points.mapModel
                delegate: PointItem { }
            }
        }
    }
}
