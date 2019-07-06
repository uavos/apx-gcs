import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Apx.Common 1.0

Item {
    id: control
    implicitWidth: textItem.implicitWidth

    property alias color: textItem.color
    Text {
        id: textItem
        anchors.fill: parent
        anchors.topMargin: font.pixelSize*0.1
        font.family: font_narrow
        font.pixelSize: Math.max(8,control.height)
        verticalAlignment: Text.AlignVCenter
        color: "#fff"
        text: ("0"+hours.toFixed()).slice(-2)+(seconds&1?":":".")+("0"+minutes.toFixed()).slice(-2) //+":"+("0"+seconds.toFixed()).slice(-2)
        property int hours: 0
        property int minutes: 0
        property int seconds: 0
        function timeChanged() {
            var date = new Date;
            hours = date.getHours()
            minutes = date.getMinutes()
            seconds = date.getUTCSeconds();
        }
        Timer {
            interval: 500; running: true; repeat: true;
            onTriggered: textItem.timeChanged()
        }
    }
    //Rectangle { anchors.fill: control; color: "#80ffffff" }
}
