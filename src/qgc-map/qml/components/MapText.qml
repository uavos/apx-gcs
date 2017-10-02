import QtQuick 2.2
//import QtGraphicalEffects 1.0
import "."


Rectangle {
    id: mapText
    color: Style.cYellow
    property int textSize: 12
    property bool bold: false
    property alias text: textItem.text
    property alias textColor: textItem.color
    property alias font: textItem.font
    width: textItem.width+2
    height: textSize*mapProvider.mapScaleFactor+1
    border.width: 0
    smooth: true
    radius: 2*mapProvider.mapScaleFactor
    Behavior on opacity { PropertyAnimation {duration: map.animation_duration*2; } }

    scale: 1/map.tmpScale

    Text {
        id: textItem
        anchors.centerIn: parent
        //anchors.verticalCenterOffset: 0
        //anchors.horizontalCenterOffset: 0
        font.pixelSize: textSize*mapProvider.mapScaleFactor
        font.bold: bold
        color: "black"
    }

    /*Glow {
        anchors.fill: textItem
        radius: 2
        samples: 4
        color: "#C0000000"
        source: textItem
    }

    layer.enabled: true

    layer.effect: Glow {
        radius: 2
        samples: radius * 2
        source: mapText
        color: "#80000000"
        transparentBorder: true
    }*/
}
