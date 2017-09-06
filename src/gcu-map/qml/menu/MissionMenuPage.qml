import QtQuick 2.2
import QtGraphicalEffects 1.0
import com.uavos.map 1.0
import "."
import "../"
import "../components"


Item {
    anchors.fill: parent
    property string caption: listModel.caption
    property var listModel
    property string icon
    Text {
        id: pageTitle
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: itemSize*1.4
        //font.bold: true
        font.family: font_narrow
        color: "white"
        text: caption
    }
    Rectangle {
        anchors.bottom: pageTitle.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        border.width: 0
        color: "#80f0f0FF"
    }
    ListView {
        anchors.top: pageTitle.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        model: listModel
        //delegate: itemDelegate
        /*Text {
            font.pixelSize: itemSize*0.8
            color: "gray"
            text: model.caption
        }*/
    }
    Component {
        id: itemDelegate
        Rectangle {
            width: parent.width
            height: itemSize*1.5
            border.width: 0
            color: mouseArea.containsMouse?bgColorHover:"transparent"
            Image {
                id: image
                anchors.fill: parent
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
    }
}
