import QtQuick 2.2
import QtGraphicalEffects 1.0
import com.uavos.map 1.0
import "."
import "../"
import "../components"


Rectangle {
    property string icon
    width: parent.width
    height: itemSize*1.5
    border.width: 0
    color: mouseArea.containsMouse?bgColorHover:"transparent"
    Image {
        id: image
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 4
        fillMode: Image.PreserveAspectFit
        source: "/icons/ionicons/"+icon+".svg"
        smooth: true
    }
    ColorOverlay {
        anchors.fill: image
        source: image
        color: "#FFFFFF"
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            console.log("tab: "+caption);
            //buttonItem.clicked();
        }
    }
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        border.width: 0
        color: "#40f0f0FF"
    }
}
