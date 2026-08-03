import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Panel {
    required property var page

    title: qsTr("SCAN")
    enabled: page.modeText === "search"
    opacity: enabled ? 1.0 : 0.5
    contentSpacing: 0

    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: "Az"; color: "#888888"; font.pixelSize: 12
            Layout.preferredWidth: 16
            Layout.preferredHeight: 24
            verticalAlignment: Text.AlignVCenter
        }
        Slider {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            implicitHeight: 24
            from: 1; to: page.scanAzRangeMax; stepSize: 1
            value: page.scanAzRange
            onValueChanged: page.scanAzRange = value
        }
        Text {
            text: page.scanAzRange.toFixed(0) + "°"
            color: "#ffffff"; font.pixelSize: 12
            Layout.preferredWidth: 30
            Layout.preferredHeight: 24
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: "El"; color: "#888888"; font.pixelSize: 12
            Layout.preferredWidth: 16
            Layout.preferredHeight: 24
            verticalAlignment: Text.AlignVCenter
        }
        Slider {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            implicitHeight: 24
            from: 1; to: page.scanElRangeMax; stepSize: 1
            value: page.scanElRange
            onValueChanged: page.scanElRange = value
        }
        Text {
            text: page.scanElRange.toFixed(0) + "°"
            color: "#ffffff"; font.pixelSize: 12
            Layout.preferredWidth: 30
            Layout.preferredHeight: 24
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
        }
    }
    ToolBtn {
        Layout.fillWidth: true
        text: qsTr("Scan")
        onClicked: page.startScan()
    }
    Text {
        visible: page.scanning && page.maxRssi > 0
        text: "ΔAz: " + (page.deltaAz >= 0 ? "+" : "") + page.deltaAz.toFixed(1)
              + "°  ΔEl: " + (page.deltaEl >= 0 ? "+" : "") + page.deltaEl.toFixed(1) + "°"
        color: "#ffaa00"; font.pixelSize: 11; font.bold: true
    }
}
