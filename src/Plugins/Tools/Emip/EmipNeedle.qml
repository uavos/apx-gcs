import QtQuick

// CircularGuage.qml rotation and SpringAnimation from the original EMIP.
// RoundScaleSmall.qml mirrors the entire dial; mirror the image, not its value.
Item {
    id: needle
    objectName: "emipRpmNeedle"
    property real value: NaN
    property real minimum: 25
    property real maximum: 115
    property real startAngle: 30
    property real spanAngle: 150
    property bool animated: true
    readonly property real targetRotation: rotationFor(value)
    readonly property real displayedRotation: image.rotation
    readonly property bool imageReady: image.status === Image.Ready

    function rotationFor(v) {
        if (!isFinite(v) || maximum <= minimum) return 69
        return v > maximum ? spanAngle + 69
             : v < minimum ? 69
             : 39 + startAngle + spanAngle / (maximum - minimum) * (v - minimum)
    }

    // rests at the minimum when there is no data
    visible: true
    transform: Scale { origin.x: needle.width / 2; xScale: -1 }
    Image {
        id: image
        width: parent.width / 1.2
        height: width
        anchors.centerIn: parent
        source: "image/needle.png"
        smooth: true
        rotation: needle.targetRotation
        Behavior on rotation {
            enabled: needle.animated && needle.visible
            SpringAnimation { spring: 3; damping: 0.8 }
        }
    }
}
