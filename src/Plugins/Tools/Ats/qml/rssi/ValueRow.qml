import QtQuick
import QtQuick.Layouts

RowLayout {
    property string label: ""
    property string value: ""
    property color valueColor: "#00ffff"

    Layout.fillWidth: true

    Text { text: label; color: "#888888"; font.pixelSize: 12 }
    Item { Layout.fillWidth: true }
    Text { text: value; color: valueColor; font.pixelSize: 13; font.bold: true }
}
