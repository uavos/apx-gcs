import QtQuick 2.2
import "./components"

GCSMenu {
    //id: menu
    //title: qsTr("Controls")

    menu: GCSMenuItem {
        GCSMenuItem { title: qsTr("Commands"); subMenu: "MenuCommands.qml"}
        GCSMenuItem { title: qsTr("HDG"); page: "HDG.qml"}
        GCSMenuItem { title: qsTr("Video"); page: "video.qml"}
        GCSMenuItem {
            title: qsTr("Settings")
            GCSMenuItem { title: qsTr("Shortcuts"); subMenu: "Shortcuts.qml" }
        }
    }

    model: ListModel {
        ListElement { title: qsTr("Commands"); subMenu: "MenuCommands.qml"}
        ListElement { title: qsTr("HDG"); page: "HDG.qml"}
        ListElement { title: qsTr("Video"); page: "video.qml"}
        ListElement {
            title: qsTr("Settings")
            contents: [
                ListElement { title: qsTr("Shortcuts"); subMenu: "Shortcuts.qml" }
            ]
        }
    }
}
