import QtQuick 2.2
import "."
import "../components"


Item {
    id: cmdCircleItem
    //property var m: vehicleItem.m
    //property var f_lat: m.field("home_lat")
    //property var f_lon: m.field("home_lon")

    property var f_cmd_north: m.field("cmd_north")
    property var f_cmd_east: m.field("cmd_east")

    property double home_x: mapProvider.lonToX(home_lon.value)
    property double home_y: mapProvider.latToY(home_lat.value)

    property double msf: mapProvider.metersToX(home_y)
    property double cmd_x: home_x+msf*cmd_east.value //mapProvider.lonToX(home_lon.value)
    property double cmd_y: home_y-msf*cmd_north.value //mapProvider.latToY(home_lat.value)

    MapImage {
        z: -99
        parent: cmdCircleItem.parent.parent
        icon: "radio-waves"
        pos: Qt.point(home_x,home_y)
        color: "black"
        //size: 22
        glow: false
        opacity: 0.2
    }
    /*MapObject {
        id: homePoint
        z: -99
        interactive: false
        //visible: mapProvider.level>10
        parent: vehicleItem.parent
        lat: f_home_lat.value
        lon: f_home_lon.value
        color: "blue"
        textColor: "white"
        opacity: 0.6
        text: "H"
    }*/

    Item{
        id: cmdPosCircle
        x: map.mapToSceneX(cmd_x)
        y: map.mapToSceneY(cmd_y)
        Rectangle {
            z: -1000
            anchors.centerIn: parent
            width: mapProvider.metersToX(cmd_y)*map.constSceneXY*2*100//f_turnR.value
            height: width
            color: "#1000FF00"
            border.color: "magenta"
            border.width: 1
            radius: width*0.5
        }
    }
}
