import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

import Apx.Common 1.0
import Apx.Controls 1.0

import GstPlayer 1.0

Item {
    id: control

    property bool interactive: false

    property var frameRect

    property var plugin: apx.tools.videostreaming
    readonly property bool alive: interactive?plugin.connectionState === GstPlayer.STATE_CONNECTED:true

    opacity: 0.7

    Item {
        id: videoFrame
        x: frameRect?frameRect.x:0
        y: frameRect?frameRect.y:0
        width: frameRect?frameRect.width:parent.width
        height: frameRect?frameRect.height:parent.height
    }


    Text {
        color: "#60FFFFFF"
        anchors.centerIn: parent
        text: qsTr("no video").toUpperCase()
        visible: !alive
        font.pixelSize: 48
        font.family: font_narrow
        font.bold: true
    }

    OverlayNumbers {
        anchors.fill: interactive?(plugin.tune.view_mode.value?control:videoFrame):control
        interactive: control.interactive
    }

    OverlayAim {
        anchors.centerIn: videoFrame
        size: Math.min(100, parent.height/10)
        visible: alive
        type: plugin?plugin.tune.overlay.aim.value:0
    }

    Connections {
        enabled: !interactive
        target: application
        onLoadingFinished: {
            control.plugin=apx.tools.videostreaming
        }
    }


}
