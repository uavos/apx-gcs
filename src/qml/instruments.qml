import QtQuick 2.2
import "./components"
import "./comm"
import "./pfd"
import "./nav"

Item {
    id: window
    //anchors.fill: parent

    property bool vertical: width/height<1.3

    Item{
        id: left_window
        anchors.fill: parent
        anchors.bottomMargin: vertical?parent.height*0.7:0
        anchors.rightMargin: vertical?0:parent.width*0.35
        //anchors.rightMargin: 0

        Item{
            id: pfd
            anchors.fill: parent

            Comm {
                id: comm
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: mandala.limit(parent.height*0.05,10,width/32)
                //source: "./comm/Comm.qml"
            }

            Pfd {
                anchors.fill: parent
                anchors.topMargin: comm.height
                showHeading: false
                //source: "./pfd/Pfd.qml"
            }

        }
    }
    Rectangle {
        id: left_split
        border.width: 1
        border.color: "#770"
        width: 1
        anchors.left: left_window.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }


    Item{
        id: bottom_window
        anchors.left: vertical?parent.left:left_split.right
        anchors.top: vertical?left_window.bottom:parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        Loader {
            anchors.fill: parent
            source: "./hdg/Hdg.qml"
        }
    }

}
