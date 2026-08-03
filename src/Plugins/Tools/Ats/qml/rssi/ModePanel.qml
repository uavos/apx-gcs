import QtQuick
import QtQuick.Layouts

Panel {
    required property var page

    title: qsTr("ATS MODE")

    ValueRow {
        label: qsTr("Current")
        value: page.modeText.toUpperCase()
        valueColor: "#44ff44"
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        ToolBtn {
            Layout.fillWidth: true
            text: qsTr("Manual")
            accent: "#2f6b3a"
            highlighted: page.modeText === "manual"
            onClicked: page.ats.setModeManual()
        }
        ToolBtn {
            Layout.fillWidth: true
            text: qsTr("Track")
            accent: "#2f6b3a"
            highlighted: page.modeText === "track"
            onClicked: page.ats.setModeTrack()
        }
        ToolBtn {
            Layout.fillWidth: true
            text: qsTr("Search")
            accent: "#2f6b3a"
            highlighted: page.modeText === "search"
            onClicked: page.ats.setModeSearch()
        }
    }
}
