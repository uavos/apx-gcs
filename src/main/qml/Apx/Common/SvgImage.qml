import QtQuick 2.2
import QtGraphicalEffects 1.0

Item {
    property alias source: image.source
    property alias sourceSize: image.sourceSize
    property alias color: imageColor.color

    property Component effect

    //smooth: ui.antialiasing
    width: image.width
    height: image.height
    Image {
        id: image
        fillMode: Image.PreserveAspectFit
        //smooth: ui.antialiasing
    }
    ColorOverlay {
        id: imageColor
        anchors.fill: image
        source: image
        color: "#FFFFFF"
        cached: true
        layer.enabled: effect
        //layer.smooth: ui.antialiasing
        layer.effect: effect
    }
}
