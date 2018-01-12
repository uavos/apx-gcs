import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import "."

MapQuickItem {  //to be used inside MapComponent only
    id: waypointItem

    property var wp: modelData

    //Fact bindings
    property real lat: wp.latitude.value
    property real lon: wp.longitude.value
    property real altitude: wp.altitude.value

    //position
    coordinate: QtPositioning.coordinate(lat,lon,altitude)

    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2
    sourceItem:
    Image {
        id: image
        source: "./icons/gcu.svg"
        sourceSize.width: 16*map.itemsScaleFactor
        sourceSize.height: width
    }
}
