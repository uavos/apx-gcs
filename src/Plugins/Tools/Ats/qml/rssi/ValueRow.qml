import QtQuick
import QtQuick.Layouts

import Apx.Common

RowLayout {
    property string label: ""
    property string value: ""
    property color valueColor: "#00ffff"

    Layout.fillWidth: true

    Text { text: label; color: "#888888"; font.pixelSize: Style.fontSize * 0.8 }
    Item { Layout.fillWidth: true }
    Text { text: value; color: valueColor; font.pixelSize: Style.fontSize * 0.85; font.bold: true }
}
