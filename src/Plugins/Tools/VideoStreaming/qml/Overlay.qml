import QtQuick 2.12
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.0

Item {
    id: root
    visible: true
    width: 100
    height: 100

    Rectangle {
        id: indicator
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        x: 5
        width: 15
        height: width
        radius: width / 2
        color: "green"
        visible: false
    }
    DropShadow {
        anchors.fill: indicator
        samples: 15
        color: "#ff000000"
        source: indicator
        cached: false
        enabled: true
        visible: true
    }


    Text {
        anchors.centerIn: parent
        text: "Hello World!"
    }

    BusyIndicator {
        anchors.centerIn: parent
    }
}
