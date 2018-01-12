import QtQuick 2.2
import QtGraphicalEffects 1.0
import "."

Item {
    id: svgImageItem
    property alias source: image.source
    property alias sourceSize: image.sourceSize
    property alias color: imageColor.color
    property alias glowColor: imageGlow.color
    property alias glow: imageGlow.visible

    width: image.width
    height: image.height
    Image {
        id: image
        //anchors.fill: parent
        //source:
        smooth: true
    }
    ColorOverlay {
        id: imageColor
        anchors.fill: image
        source: image
        color: "#FFFFFF"
    }
    Glow {
        id: imageGlow
        anchors.fill: image
        radius: 4
        samples: 8
        color: "#000000"
        source: imageColor
    }
    FastBlur {
        id: imageBlur
        anchors.fill: image
        radius: Math.max(width,height)/10
        source: imageColor
        visible: !imageGlow.visible
    }
}
