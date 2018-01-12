import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import "./helper.js" as Helper

Item {
    id: scale
    visible: scaleText.text != "0 m"
    height: scaleText.height * 2
    width: scaleImage.width
    property color color: "#fff"

    Rectangle {
        id: scaleImageLeft
        //source: "../resources/scale_end.png"
        border.width: 0
        color: scale.color
        width: 1
        height: 8
        anchors.bottom: parent.bottom
        anchors.right: scaleImage.left
    }
    Rectangle {
        id: scaleImage
        //source: "../resources/scale.png"
        border.width: 0
        color: scale.color
        implicitWidth: 100
        height: 1
        anchors.bottom: parent.bottom
        anchors.right: scaleImageRight.left
    }
    Rectangle {
        id: scaleImageRight
        //source: "../resources/scale_end.png"
        border.width: 0
        color: scale.color
        width: 1
        height: 8
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }
    Label {
        id: scaleText
        color: scale.color //"#004EAE"
        anchors.centerIn: parent
        text: "0 m"
    }
    Component.onCompleted: {
        calculateScale();
    }

    Timer {
        id: scaleTimer
        interval: 100
        running: false
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
        scaleTimer.restart()
    }

    property variant scaleLengths: [5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]

    function calculateScale()
    {
        var coord1, coord2, dist, text, f
        f = 0
        coord1 = map.toCoordinate(Qt.point(0,scale.y))
        coord2 = map.toCoordinate(Qt.point(0+scaleImage.implicitWidth,scale.y))
        dist = Math.round(coord1.distanceTo(coord2))

        if (dist === 0) {
            // not visible
        } else {
            for (var i = 0; i < scaleLengths.length-1; i++) {
                if (dist < (scaleLengths[i] + scaleLengths[i+1]) / 2 ) {
                    f = scaleLengths[i] / dist
                    dist = scaleLengths[i]
                    break;
                }
            }
            if (f === 0) {
                f = dist / scaleLengths[i]
                dist = scaleLengths[i]
            }
        }

        text = Helper.formatDistance(dist)
        scaleImage.width = (scaleImage.implicitWidth * f) - 2 * scaleImageLeft.width
        scaleText.text = text
    }
}
