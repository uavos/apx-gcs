import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Button {
    id: btn

    property color accent: "#7a3030"

    implicitHeight: 32
    implicitWidth: Math.max(44, btnText.implicitWidth + leftPadding + rightPadding)
    leftPadding: 12
    rightPadding: 12
    font.pixelSize: 13
    opacity: enabled ? 1.0 : 0.4

    contentItem: Text {
        id: btnText
        text: btn.text
        font: btn.font
        color: "#e0e0e0"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 4
        color: btn.down ? "#4a5568" : (btn.highlighted ? btn.accent : (btn.hovered ? "#3a4358" : "#2d3548"))
        border.color: "#4a5568"
        border.width: 1
    }
}
