import QtQuick 2.2
import "../common"

Item {
    id: numValue
    property string label
    property string value
    property string toolTip
    property string labelColor: "gray"
    property string valueColor: "white"
    property double anumation_duration: ui.smooth?200:0
    property string valueFont: font_narrow //font_mono
    width: numValue_label.width+numValue_value.width+(value?8:0)
    Text {
        id: numValue_label
        //smooth: ui.antialiasing
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: label
        //text:  "<pre>"+label+"</pre>"
        font.pixelSize: parent.height*0.8   //*0.6
        font.family: "Monospace" //font_mono
        //font.bold: true
        color: labelColor
    }
    Text {
        id: numValue_value
        anchors.left: numValue_label.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: value //(value/10).toFixed()
        //text:  "<pre>"+value+"</pre>"
        font.pixelSize: parent.height   //*0.99
        font.family: valueFont
        font.bold: valueFont===font_mono
        color: valueColor
    }
    Rectangle{
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#555"
    }

    ToolTipArea {
        text: numValue.toolTip
    }
}

