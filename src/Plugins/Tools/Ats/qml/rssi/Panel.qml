import QtQuick
import QtQuick.Layouts

Rectangle {
    id: panel

    property string title: ""
    default property alias content: contentCol.data
    property alias contentSpacing: contentCol.spacing

    Layout.fillWidth: true
    implicitHeight: titleText.implicitHeight + contentCol.implicitHeight + 15
    color: "#141a29"
    radius: 6
    border.color: "#2a3448"
    border.width: 1

    Text {
        id: titleText
        text: panel.title
        color: "#8899bb"
        font.pixelSize: 11
        font.bold: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 6
    }
    ColumnLayout {
        id: contentCol
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 6
        anchors.topMargin: 3
        spacing: 2
    }
}
