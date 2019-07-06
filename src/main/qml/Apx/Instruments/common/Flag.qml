import QtQuick 2.2
import "."

Rectangle {
    id: flag
    property string text
    property bool show: true
    property bool blinking: false
    property color flagColor: "yellow"
    property variant control
    property string toolTip
    property bool inverted: false

    property bool blink: true

    color: inverted?"black":((ui.test||show)&&blink)?flagColor:"transparent" //flagColor //(show&&blink)?flagColor:"transparent"
    width: height/0.35
    onBlinkingChanged: blink=true

    opacity: ((ui.test||show)&&blink)?1:0
    //visible: ((ui.test||show)&&blink)?1:0

    SequentialAnimation on show {
        running: blinking
        loops: Animation.Infinite
        PropertyAction { target: flag; property: "blink"; value: true }
        PauseAnimation { duration: 300 }
        PropertyAction { target: flag; property: "blink"; value: false }
        PauseAnimation { duration: 300 }
    }
    Text {
        id: text
        anchors.fill: parent
        anchors.leftMargin: 1
        text: flag.text
        font.pixelSize: Math.max(8,height)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: font_condenced
        color: inverted?flagColor:"black"
        font.bold: inverted
    }
    /*MouseArea {
        anchors.fill: parent
        enabled: control?true:false
        onClicked: {
            control.setValue(!control.value)
        }
    }*/
    ToolTipArea {
        enabled: flag.show
        text: flag.toolTip
    }
}

