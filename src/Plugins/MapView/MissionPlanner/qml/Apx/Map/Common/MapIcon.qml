import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapQuickItem {
    id: control

    property alias name: icon.name

    property alias color: icon.color
    property alias size: icon.size

    visible: coordinate.isValid && coordinate!=QtPositioning.coordinate(0,0,0)

    //constants
    anchorPoint.x: icon.implicitWidth/2
    anchorPoint.y: icon.implicitHeight/2

    sourceItem: MaterialIcon {
        id: icon
        size: 32
    }
}
