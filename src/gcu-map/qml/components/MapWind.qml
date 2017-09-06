import QtQuick 2.2
import QtGraphicalEffects 1.0
import "."
import "../pfd"
import "../components"


Item {
    id: wind
    width: Math.max(arrow.height,text.width)
    height: text.height+arrow.height
    MapText {
        id: text
        scale: 1
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: windSpd.value.toFixed(1)+"m/s"
        bold: true
        textSize: 16
    }

    Glow {
        id: arrow
        anchors.top: text.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: height
        height: 60*mapProvider.mapScaleFactor
        radius: 8
        samples: 16
        color: "black"//"#F0000000"
        source: ColorOverlay {
            width: height
            height: arrow.height
            source: PfdImage {
                width: height
                height: arrow.height
                elementName: "wind-arrow"
                smooth: true
                border: 5
                fillMode: Image.PreserveAspectFit
            }
            color: "yellow"
        }
        rotation: windHdg.value
        Behavior on rotation { RotationAnimation {duration: 1000; direction: RotationAnimation.Shortest; } }
    }
}
