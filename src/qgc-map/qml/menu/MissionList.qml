import QtQuick 2.2
import QtGraphicalEffects 1.0
import com.uavos.map 1.0
import "."
import "../"
import "../components"


Item {
    id: missionList
    property color bgColor: "#A0000000"
    property color bgColorHover: "#FF404040"
    property double itemHeight: Math.max(Math.min(width,height)/50,Math.max(20,15*mapProvider.mapScaleFactor))
    property int minWidth: Math.min(parent.width,titleText.width+itemHeight*2)
    property bool off: mission.empty


    //state: "off"
    states: [
        State {
            name: "empty"; when: mission.empty
            PropertyChanges { target: listRect; height: 0; opacity: 0; }
        },
        State {
            name: "up"; extend: "empty"; when: mission.empty===false
            PropertyChanges { target: listRect; height: sz; opacity: 1; }
        },
        State {
            name: "down"; extend: "up"; //when: mouseArea.pressed == true
            PropertyChanges { target: listRect; height: sz; opacity: 1; }
        }
    ]
    transitions: Transition {
        to: "*"
        ParallelAnimation {
            NumberAnimation { properties: "height,opacity"; duration: 200; easing.type: Easing.InOutQuad }
        }
    }

    //top mission title bar
    Rectangle {
        id: titleRect
        anchors.top: parent.top
        anchors.left: parent.left
        width: titleText.width+itemHeight*4
        height: itemHeight
        border.width: 0
        color: mouseArea.containsMouse?bgColorHover:bgColor
        radius: height/2
        clip: true
        Text {
            id: titleText
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -1
            font.pixelSize: parent.height*0.8
            font.bold: true
            color: "white"
            text: mission.name + (" ("+missionList.state+")")
        }
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if(mission.empty){
                    mission.request();
                }
            }
        }
    }

    //map objects list window
    Item {
        anchors.top: titleRect.bottom
        anchors.topMargin: itemHeight*0.1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        Rectangle {
            id: listRect
            property int sz: parent.height
            anchors.top: parent.top
            anchors.left: parent.left
            width: Math.min(parent.width,itemHeight*10)
            height: sz
            border.width: 0
            color: bgColor
            radius: itemHeight/2
            clip: true
            Row {
                id: buttonsBar
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: width/buttonsBar.children.length
                MapButton {
                    size: parent.height
                    icon: "android-upload"
                    //onClicked: map.zoomOut(mapProvider.center);
                }
                MapButton {
                    size: parent.height
                    icon: "android-download"
                    //onClicked: mission.clear();
                }
                MapButton {
                    size: parent.height
                    icon: "folder"
                    //onClicked: map.zoomIn(mapProvider.center);
                }
                MapButton {
                    size: parent.height
                    icon: "archive"
                    //onClicked: mission.request();
                }
                MapButton {
                    size: parent.height
                    icon: "android-arrow-dropright"
                    onClicked: mission.clear();
                }
            }
        }
    }
}
