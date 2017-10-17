import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import QtQuick.Window 2.2

import "qrc:///components"

ApplicationWindow {
    width: 1000
    height: 600
    visible: true
    title: app_title    //context property

    Loader {
        asynchronous: app.settings.smooth.value
        anchors.fill: parent
        source: app_source //context property
    }
}

