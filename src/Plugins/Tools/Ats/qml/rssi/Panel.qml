import QtQuick
import QtQuick.Layouts

import Apx.Common

Rectangle {
    id: panel

    property string title: ""
    default property alias content: contentCol.data
    property alias contentSpacing: contentCol.spacing

    readonly property real margins: Style.spacing * 1.5

    Layout.fillWidth: true
    implicitHeight: titleText.implicitHeight + contentCol.implicitHeight + margins * 3
    color: "#141a29"
    radius: 4
    border.color: "#2a3448"
    border.width: 1

    Text {
        id: titleText
        text: panel.title
        color: "#8899bb"
        font.pixelSize: Style.fontSize * 0.65
        font.bold: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: panel.margins
    }
    ColumnLayout {
        id: contentCol
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: panel.margins
        anchors.topMargin: panel.margins / 2
        spacing: 2
    }
}
