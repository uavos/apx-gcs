import QtQuick 2.2
import QtQuick.Controls 1.1
import QtGraphicalEffects 1.0
import com.uavos.map 1.0
import "."
import "../"
import "../components"


Item {
    id: missionMenu
    property color bgColor: "#B0000000"
    property color bgColorHover: "#FF404040"
    property double itemSize: Math.max(Math.min(width,height)/50,Math.max(20,15*mapProvider.mapScaleFactor))
    property bool open: false

    /*VisualItemModel {
        id: itemModel
        MissionMenuMain { }
        MissionMenuPage {
            caption: qsTr("Runways")
            icon: "android-plane"
        }
    }*/

    /*ListModel {
        id:listModel
        ListElement {
            caption: qsTr("Mission");
            icon: "android-menu"
            elements: [
                ListElement {
                    caption: "second2"
                    elements: [
                        ListElement {
                            caption: "second2.2"
                            elements: []
                        }
                    ]
                },
                ListElement {caption: "second3"; elements: []},
                ListElement {caption: "second4"; elements: []}
            ]
        }
        //ListElement { icon: "android-upload" }
        //ListElement { icon: "android-download" }
        //ListElement { icon: "android-folder" }

        ListElement {
            caption: qsTr("Runways")
            icon: "android-plane"
            page: "RW"
        }
        ListElement {
            caption: qsTr("Waypoints")
            icon: "ios-location"
            page: "WP"
            //cmp: MissionMenuPage {}
        }
        ListElement { icon: "eye" }
        ListElement { icon: "nuclear" }
        ListElement { icon: "ios-nutrition" }
    }*/

    ListModel {
        id:listModel
        ListElement { icon: "android-menu" }
        ListElement { icon: "android-locate" }
        ListElement { icon: "ios-plus" }
        ListElement { icon: "ios-minus" }
        ListElement { icon: "ios-list" }
    }

    function pageUrl(page)
    {
        return Qt.resolvedUrl("MissionMenu"+page+".qml")
    }

    Rectangle {
        id: bgRect
        property int sz: parent.height//*0.8
        anchors.top: parent.top
        anchors.left: parent.left
        //anchors.topMargin: -radius
        //anchors.leftMargin: -radius
        width: itemSize*2//Math.min(parent.width,itemSize*10)
        height: sz-anchors.topMargin
        border.width: 0
        color: "black"//bgColor
        //radius: itemSize/4
        clip: true
        Item {
            anchors.fill: parent
            //anchors.leftMargin: -parent.anchors.leftMargin
            //anchors.topMargin: -parent.anchors.topMargin
            //left tabs bar
            Item{
                id: tabs
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: itemSize*2
                ListView {
                    model: listModel
                    anchors.fill: parent
                    snapMode: ListView.SnapToItem
                    delegate: tabDelegate
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    border.width: 0
                    color: "#80f0f0FF"
                }
            }
            //Page
            /*StackView {
                id: stackView
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: tabs.right
                anchors.right: parent.right
                focus: true
                Keys.onReleased: if (event.key === Qt.Key_Back && stackView.depth > 1) {
                                     stackView.pop()
                                     event.accepted = true;
                                 }
                initialItem: pageUrl("Main")
            }*/

            /*Item {
                id: page
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: tabs.right
                anchors.right: parent.right
                Text {
                    id: pageTitle
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: itemSize*1.4
                    font.bold: true
                    color: "white"
                    text: "title"
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
                    model: listModel
                    delegate: Text {
                        font.pixelSize: itemSize
                        color: "gray"
                        text: caption
                    }
                }
            }*/
        }
    }
    Component {
        id: tabDelegate
        Rectangle {
            width: parent.width
            height: parent.width*1.5
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
