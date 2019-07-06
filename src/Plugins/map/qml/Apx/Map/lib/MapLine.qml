import QtQuick 2.5;
import QtLocation 5.9
import QtPositioning 5.6

MapPolyline {
    id: line
    property var p1: QtPositioning.coordinate()
    property var p2: QtPositioning.coordinate()

    //smooth: ui.antialiasing
    line.width: 1
    line.color: "white"

    function updatePath()
    {
        line.replaceCoordinate(0,p1)
        line.replaceCoordinate(1,p2)
    }
    onP1Changed: updatePath()
    onP2Changed: updatePath()
    Component.onCompleted: {
        line.addCoordinate(p1)
        line.addCoordinate(p2)
        updatePath()
    }
}
