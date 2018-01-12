import QtQuick 2.5;
import "."

Rectangle {
    id: mapText

    property alias text: textItem.text
    property alias font: textItem.font
    property alias horizontalAlignment: textItem.horizontalAlignment
    property alias verticalAlignment: textItem.verticalAlignment
    property alias textColor: textItem.color

    Behavior on width { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }
    Behavior on height { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }

    border.width: 0
    color: "gray"
    smooth: true
    width: textItem.contentWidth+5
    height: textItem.contentHeight+5
    radius: 3
    clip: true
    Text {
        id: textItem
        x: 2
        y: 2
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
}
