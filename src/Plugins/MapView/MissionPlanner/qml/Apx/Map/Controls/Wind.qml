import QtQuick          2.12

import Apx.Common 1.0

Item {
    id: control

    readonly property real m_wspd: mandala.est.calc.wspd.value
    readonly property real m_whdg: mandala.est.calc.whdg.value

    implicitWidth: 100
    implicitHeight: implicitWidth

    readonly property int spacing: 0
    readonly property int fontSize: 28*ui.scale

    //internal
    readonly property int arrowSize: height-textItem.height-spacing

    property bool smoothMove: true
    Timer {
        id: smoothMoveTimer
        running: true
        interval: 1000
        onTriggered: smoothMove=true
    }
    Connections {
        target: apx.vehicles
        onVehicleSelected: {
            smoothMove=false
            smoothMoveTimer.restart()
        }
    }

    Text {
        id: textItem
        color: "#fff"
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: m_wspd.toFixed(1)
        font.family: font_narrow
        font.pixelSize: fontSize
        font.bold: true
    }
    Text {
        id: textHdg
        color: "#fff"
        text: (Math.round(apx.angle360(image.v+180)/5)*5).toFixed(0)
        font.family: font_narrow
        font.pixelSize: fontSize*0.6
        rotation: image.rotation
        anchors.centerIn: image
    }
    SvgImage {
        id: image
        color: "#fff" //"#fd6"
        source: "../icons/wind-arrow.svg"
        sourceSize.height: Math.round(arrowSize)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        rotation: v-map.bearing
        property real v: m_whdg
        Behavior on v { enabled: ui.smooth && control.smoothMove; RotationAnimation {duration: 1000; direction: RotationAnimation.Shortest; } }
    }
}
