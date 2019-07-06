import QtQuick 2.2
import "."

Rectangle {
    id: rectValue
    property string value
    property string toolTip
    color: "#30000000"
    //color: "#225d9d"
    border.width: 1
    border.color: "white"
    Text {
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
        color: "magenta"
    }
    ToolTipArea {
        text: rectValue.toolTip
    }
}

