import QtQuick 2.12
import QtQuick.Controls 2.12

Label {
    id: item
    color: invert?itemColor:"#000"
    font.family: font_condenced
    font.pixelSize: fontSize*0.9

    property bool invert: false

    property var itemColor: "#aaa"

    background: Rectangle {
        border.width: 0 // invert?1:0
        //border.color: itemColor
        color: invert?"#000":itemColor
        radius: 2
    }
}

