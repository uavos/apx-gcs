import QtQuick 2.5
import APX.Vehicles 1.0
import QtGraphicalEffects 1.0
import QtQml 2.12

import Apx.Common 1.0

import "../lib"

Item {
    id: control


    property int fontSize: 28*ui.scale

    //Fact bindings
    readonly property real f_windSpd: m.windSpd.value
    readonly property real f_windHdg: m.windHdg.value

    //internal
    readonly property int arrowSize: width*0.9
    width: visible?textItem.height*1.7:0
    height: visible?textItem.height+10+arrowSize:0

    visible: f_windSpd>0

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
        text: f_windSpd.toFixed(1)
        font.family: font_narrow
        font.pixelSize: fontSize
        font.bold: true
    }
    Text {
        id: textHdg
        visible: f_windSpd>1
        color: "#fff"
        //anchors.verticalCenter: textItem.bottom
        //anchors.horizontalCenter: parent.horizontalCenter
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
        property real v: f_windHdg
        Behavior on v { enabled: ui.smooth && control.smoothMove; RotationAnimation {duration: 1000; direction: RotationAnimation.Shortest; } }
    }
}
