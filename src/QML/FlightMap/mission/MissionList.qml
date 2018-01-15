import QtQuick 2.5;
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import "."
import ".."

ListView {
    id: missionList
    model: app.vehicles.current.mission.waypoints.model
    implicitWidth: contentItem.childrenRect.width
    orientation: ListView.Vertical
    spacing: 2
    clip: true
    snapMode: ListView.SnapToItem

    delegate: MapText {
        text: modelData?(modelData.title+" "+modelData.type.text.slice(0,1)+" "+modelData.altitude.text):""
    }
}
