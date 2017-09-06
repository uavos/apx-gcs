import QtQuick 2.2

Rectangle {
    id: flag
    property string text
    property bool show: true
    property bool blinking: false
    property color flagColor: "yellow"
    property string toolTip

    property bool blink: true
    color: (show&&blink)?flagColor:"gray"
    width: height*2
    border.width: 2
    border.color: (show&&blink)?flagColor:"#555"
    radius: height*0.2
    onBlinkingChanged: blink=true

    SequentialAnimation on show {
        running: blinking
        loops: Animation.Infinite
        PropertyAction { target: flag; property: "blink"; value: true }
        PauseAnimation { duration: 200 }
        PropertyAction { target: flag; property: "blink"; value: false }
        PauseAnimation { duration: 200 }
    }
    Rectangle {
        anchors.fill: parent
        //border.width: 2
        //border.color: "#555"
        radius: flag.radius
        color: "#60000000"
    }
    Text {
        id: text
        //visible: (show&&blink)
        color: (show&&blink)?flag.flagColor:"#90000000"
        anchors.fill: parent
        anchors.leftMargin: 1
        anchors.topMargin: 1
        text: flag.text
        //text:  "<pre>"+flag.text+"</pre>"
        font.pixelSize: height
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: font_narrow
        //font.bold: true
    }
    ToolTipArea {
        text: flag.toolTip
    }

}

