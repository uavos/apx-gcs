import QtQuick 2.2
import com.uavos.map 1.0
import "."
import "../"
import "../components"


Item {
    id: missionList
    property double textSize: Math.max(14,12*mapProvider.mapScaleFactor)
    property double itemHeight: textSize+margin
    property double margin: 4
    Rectangle {
        id: rect
        anchors.top: parent.top
        anchors.left: parent.left
        width: Math.min(parent.width,textSize*16)
        height: itemHeight
        border.width: 0
        color: "#C0000000"
        radius: itemHeight/2
        Item {
            anchors.fill: parent
            anchors.margins: margin/2

            Text {
                id: titleText
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                verticalAlignment: Text.AlignTop
                font.pixelSize: textSize
                font.bold: true
                color: "white"
                text: mission.name
            }
        }
    }
}
