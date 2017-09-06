import QtQuick 2.2
import QtGraphicalEffects 1.0
import "."

Item {
    id: buttonItem
    property double size: 32*mapProvider.mapScaleFactor
    width: size
    height: size
    property string icons: "ionicons"
    property string icon
    property point pos
    property alias color: imageColor.color
    property alias glowColor: imageGlow.color
    property alias glow: imageGlow.visible

    scale: 1/map.tmpScale //zoom no scale

    x: map.mapToSceneX(pos.x)-width/2
    y: map.mapToSceneY(pos.y)-height/2

    Image {
        id: image
        anchors.fill: parent
        source: "/icons/"+icons+"/"+icon+".svg"
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
        radius: size/10
        source: imageColor
        visible: !imageGlow.visible
    }
}
