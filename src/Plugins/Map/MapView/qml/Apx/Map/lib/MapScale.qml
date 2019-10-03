import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtQml 2.12

import Apx.Common 1.0

Item {
    id: control

    implicitWidth: 100

    height: width*0.18

    property color color: "#fff"

    property int alignment: Qt.AlignRight


    property real value: 0
    visible: value > 0

    readonly property int lineWidth: Math.max(1,1*ui.scale)
    readonly property int markSize: control.height*0.5

    BoundingRect { }

    Rectangle {
        id: scaleImage
        border.width: 0
        color: control.color
        height: lineWidth
        anchors.bottom: parent.bottom
        anchors.right: control.right
    }

    Rectangle {
        border.width: 0
        color: control.color
        width: lineWidth
        height: markSize
        anchors.bottom: scaleImage.bottom
        anchors.left: scaleImage.left
    }
    Rectangle {
        border.width: 0
        color: control.color
        width: lineWidth
        height: markSize
        anchors.bottom: scaleImage.bottom
        anchors.right: scaleImage.right
    }

    Label {
        id: scaleText
        color: control.color //"#004EAE"
        anchors.top: parent.top
        anchors.horizontalCenter: scaleImage.horizontalCenter
        text: Apx.formatDistance(control.value)
        font.pixelSize: control.height*0.7
    }

    Timer {
        id: scaleTimer
        interval: 1000
        running: true
        repeat: false
        onTriggered: {
            calculateScale()
        }
    }

    Connections {
        target: map
        onCenterChanged: update()
        onZoomLevelChanged: update()
        onWidthChanged: update()
        onHeightChanged: update()
    }

    function update()
    {
        scaleTimer.start()
    }

    readonly property var scaleLengths: [5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000, 5000000, 10000000, 100000000]

    function calculateScale()
    {
        var coord1, coord2, dist, f
        f = 1
        coord1 = map.toCoordinate(Qt.point(control.x,control.y))
        coord2 = map.toCoordinate(Qt.point(control.x+control.width,control.y))
        dist = Math.round(coord1.distanceTo(coord2))

        if (dist > 0) {
            for (var i = scaleLengths.length-1; i>=0; i--) {
                if (dist < scaleLengths[i]) continue
                f = scaleLengths[i] / dist
                dist = scaleLengths[i]
                break;
            }
        }
        control.value=dist
        scaleImage.width = control.width * f
    }
}
