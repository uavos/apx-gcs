import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

MapQuickItem {
    id: control

    property string iconName

    property color color: "#fff"
    property int size: 32

    visible: coordinate.isValid && coordinate!=QtPositioning.coordinate(0,0,0)

    //constants
    anchorPoint.x: icon.implicitWidth/2
    anchorPoint.y: icon.implicitHeight/2

    sourceItem: Text {
        id: icon
        font.family: "Material Design Icons"
        font.pixelSize: control.size
        text: materialIconChar[iconName]
        color: control.color
    }
}
