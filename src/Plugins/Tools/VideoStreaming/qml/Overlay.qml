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
    property bool loaded: plugin?true:false

    property bool showNumbers: true
    property bool showAim: true


    opacity: ui.effects?0.7:1
    MouseArea {
        anchors.fill: parent
        onClicked: control.forceActiveFocus()
    }

    Item {
        id: videoFrame
        x: (frameRect?frameRect.x:0)-control.x
        y: (frameRect?frameRect.y:0)-control.y
        width: frameRect?frameRect.width:parent.width
        height: frameRect?frameRect.height:parent.height
    }


    /*Loader {
        active: !alive
        anchors.centerIn: videoFrame
        anchors.verticalCenterOffset: -parent.height/6
        sourceComponent: Text {
            color: "#60FFFFFF"
            text: qsTr("no video").toUpperCase()
            font.pixelSize: 48
            font.family: font_narrow
            font.bold: true
        }
    }*/
    Loader {
        active: !alive
        anchors.centerIn: videoFrame
        sourceComponent: MaterialIcon {
            color: "#60FFFFFF"
            name: "video-off-outline"
            size: 64
        }
    }


    Loader {
        active: control.loaded && showNumbers
        anchors.fill: interactive?(loaded && apx.tools.videostreaming.tune.view_mode.value>0?control:videoFrame):control
        anchors.margins: 10
        sourceComponent: OverlayNumbers {
            id: numbers
            interactive: control.interactive
            alive: control.alive

            OverlayGimbal {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: numbers.overlayItemSize*1.1
                width: Math.min(80, parent.height/5)
            }
        }
    }

    property int aimSize: Math.max(60,Math.min(100, control.height/10))
    Loader {
        active: control.loaded && alive && showAim
        anchors.centerIn: videoFrame
        sourceComponent: OverlayAim {
            size: aimSize
            type: apx.tools.videostreaming.tune.overlay.aim.value
        }
    }

    Loader {
        active: control.loaded && interactive && apx.tools.videostreaming.tune.controls.value
        anchors.fill: videoFrame
        sourceComponent: CamControls {
            size: aimSize
        }
    }


    Connections {
        enabled: true //!interactive
        target: application
        onLoadingFinished: {
            control.plugin=apx.tools.videostreaming
            control.loaded=true
            //console.log(control.plugin)
        }
    }


}
