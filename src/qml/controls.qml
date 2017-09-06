import QtQuick 2.2
import QtQuick.Controls 1.0
import "./components"

Item {
    Pages {
        id: pages
        anchors.fill: parent
        title: qsTr("Controls")
        saveState: true

        listModel: ListModel {
            ListElement {
                title: qsTr("HDG")
                page: "HDG.qml"
            }
            ListElement {
                title: qsTr("Video")
                page: "video.qml"
            }
        }
    }

}
