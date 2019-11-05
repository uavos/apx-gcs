import QtQuick 2.12
import QtQuick.Controls 2.5

import Apx.Common 1.0
import Apx.Controls 1.0

Item {
    id: root

    /*BusyIndicator {
        anchors.centerIn: parent
    }*/

    property bool interactive: false

    opacity: 0.7


    NumbersBar {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        showEditButton: interactive
        settingsName: "video_bottom"
        defaults: [
            {"bind": "altitude", "title": "ALT", "prec": "0"},
        ]
    }
}
