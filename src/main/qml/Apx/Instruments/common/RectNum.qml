import QtQuick 2.2
import "."

Rectangle {
    id: _control
    property string value
    property string toolTip
    color: enabled?"#30000000":"transparent"
    //color: "#225d9d"
    border.width: 1
    border.color: "white"

    property alias textColor: _text.color

    property bool enabled: true

    Text {
        id: _text
        anchors.fill: parent
        anchors.margins: 1
        anchors.topMargin: 2
        anchors.rightMargin: 1
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text:  value
        font.pixelSize: height
        font.family: font_mono
        font.bold: true
        color: _control.enabled?"magenta":"gray"
    }
    ToolTipArea {
        text: _control.toolTip
    }
}

