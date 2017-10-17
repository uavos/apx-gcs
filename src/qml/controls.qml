import QtQuick 2.2
import QtQml 2.2
import "./menu"

GCSMenu {
    id: window
    title: qsTr("Controls")

    fields: GCSMenuModel {
        GCSMenuField { title: qsTr("Commands"); pageMenu: "menu/MenuCommands.qml"}

        GCSMenuField { title: qsTr("Instruments"); separator: true; }
        GCSMenuField { title: qsTr("HDG"); page: "HDG.qml"}
        GCSMenuField { title: qsTr("Video"); page: "video.qml"}

        GCSMenuField { title: qsTr("System"); separator: true; }
        GCSMenuField { title: qsTr("Preferences"); pageMenu: "menu/MenuPreferences.qml" }

        //GCSMenuField { title: app.prefs.title; }
        GCSMenuField { fact: app; }
    }
}
