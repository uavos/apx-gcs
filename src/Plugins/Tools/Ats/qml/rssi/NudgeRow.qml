import QtQuick
import QtQuick.Layouts

import Apx.Common

// value with -/+ buttons, 1 deg step
RowLayout {
    property string label: ""
    property real value: 0
    readonly property real step: 1

    signal nudge(real d)

    Layout.fillWidth: true
    spacing: 4

    Text {
        text: label
        color: "#888888"
        font.pixelSize: Style.fontSize * 0.8
        Layout.preferredWidth: Style.fontSize
    }
    Text {
        text: Math.round(value) + "°"
        color: "#00ffff"
        font.pixelSize: Style.fontSize * 0.85
        font.bold: true
        Layout.preferredWidth: Style.fontSize * 2.5
    }
    ToolBtn {
        Layout.fillWidth: true
        text: "-"
        onClicked: nudge(-step)
    }
    ToolBtn {
        Layout.fillWidth: true
        text: "+"
        onClicked: nudge(step)
    }
}
