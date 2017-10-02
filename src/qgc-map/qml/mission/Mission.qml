import QtQuick 2.2
import com.uavos.map 1.0
import "."
import "../"


Item {
    id: missionItem
    Repeater {
        model: mission.taxiways
        Taxiway {}
    }
    Repeater {
        model: mission.points
        Poi {}
    }
    Repeater {
        model: mission.waypoints
        Waypoint {}
    }
    Repeater {
        model: mission.runways
        Runway {}
    }
}
