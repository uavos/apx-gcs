import QtQuick
import QtQuick.Layouts

import Apx.Common

Panel {
    required property var page

    // mode set by the operator, the buttons are highlighted by it
    readonly property string setMode: page.modeFact ? page.modeFact.text : ""
    // reported mode differs from the set one while the command is not applied
    readonly property bool pending: setMode !== "AUTO" && page.modeText.toUpperCase() !== setMode

    title: qsTr("ATS MODE")

    ValueRow {
        label: qsTr("Current")
        value: page.modeText.toUpperCase()
        valueColor: pending ? "#ffaa00" : "#44ff44"
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        ToolBtn {
            Layout.fillWidth: true
            text: qsTr("Auto")
            accent: "#2f6b3a"
            highlighted: setMode === "AUTO"
            onClicked: page.modeFact.setValue("AUTO")
        }
        ToolBtn {
            Layout.fillWidth: true
            text: qsTr("Manual")
            accent: "#2f6b3a"
            highlighted: setMode === "MANUAL"
            onClicked: page.modeFact.setValue("MANUAL")
        }
        ToolBtn {
            Layout.fillWidth: true
            text: qsTr("Track")
            accent: "#2f6b3a"
            highlighted: setMode === "TRACK"
            onClicked: page.modeFact.setValue("TRACK")
        }
    }
}
