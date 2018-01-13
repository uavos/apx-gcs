import QtQuick 2.5;
import QtLocation 5.9
import QtPositioning 5.6
import ".."

MapQuickItem {  //to be used inside MapComponent only
    id: vehicleItem

    //Fact bindings
    property real home_lat: m.home_lat.value
    property real home_lon: m.home_lon.value


    coordinate: QtPositioning.coordinate(home_lat,home_lon)

    //constants
    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2

    sourceItem:
    MapSvgImage {
        id: image
        source: "../icons/home.svg"
        sourceSize.width: 16*map.itemsScaleFactor
        sourceSize.height: width
        color: "#000"
        glowColor: "#0f0"
    }

}
