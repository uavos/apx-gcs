import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

import Apx.Common 1.0
import Apx.Controls 1.0

Item {
    id: control

    property bool interactive: false

    property var frameRect

    property var plugin: apx.tools.videostreaming
    property bool alive: true

    opacity: ui.effects?0.7:1

    Item {
        id: videoFrame
        x: (frameRect?frameRect.x:0)-control.x
        y: (frameRect?frameRect.y:0)-control.y
        width: frameRect?frameRect.width:parent.width
        height: frameRect?frameRect.height:parent.height
    }


    Loader {
        active: !alive
        anchors.centerIn: videoFrame
        sourceComponent: Text {
            color: "#60FFFFFF"
            text: qsTr("no video").toUpperCase()
            font.pixelSize: 48
            font.family: font_narrow
            font.bold: true
        }
    }

    OverlayNumbers {
        id: numbers
        anchors.fill: interactive?(plugin.tune.view_mode.value>0?control:videoFrame):control
        interactive: control.interactive
        alive: control.alive

        Loader {
            active: true
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: numbers.overlayItemSize*1.1
            width: Math.min(80, parent.height/5)
            sourceComponent: OverlayGimbal { }
        }
    }

    Loader {
        active: alive
        anchors.centerIn: videoFrame
        sourceComponent: OverlayAim {
            size: Math.min(100, control.height/10)
            type: plugin.tune.overlay.aim.value
        }
    }


    Connections {
        enabled: !interactive
        target: application
        onLoadingFinished: {
            control.plugin=apx.tools.videostreaming
        }
    }


}
