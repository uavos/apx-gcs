import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0

import "qrc:///components"

Item {
    id: window
    //anchors.fill: parent
    //width: 400
    //height: 240
    //visible: true

    //Material.theme: Material.Dark
    //Material.primary: "#41cd52"
    //Material.accent: "#41cd52"
    //Material.background: Material.Teal


    Rectangle {
        color: "transparent"
        anchors.fill: parent
        Loader {
            anchors.fill: parent
            source: "./EFIS.qml"
        }
    }





    /*Settings {
        category: "qgc"
        property alias x: window.x
        property alias y: window.y
        property alias width: window.width
        property alias height: window.height
    }*/

}
