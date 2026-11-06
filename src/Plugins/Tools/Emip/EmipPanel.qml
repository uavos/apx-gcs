import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: panel
    objectName: "emipPanel"
    property var telemetry: null
    property var config: null
    readonly property int indicatorCount: telemetry ? telemetry.annunciators.length : 0
    implicitWidth: 600
    implicitHeight: 300
    color: "#000000"
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4
        GridLayout {
            objectName: "emipIndicatorGrid"
            columns: 3
            rowSpacing: 5
            columnSpacing: 5
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(26, Math.min(44, panel.height * 0.12))
            Layout.minimumHeight: Layout.preferredHeight
            Layout.maximumHeight: Layout.preferredHeight
            Repeater {
                model: panel.telemetry ? panel.telemetry.annunciators : []
                LightIndication {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    textData: modelData.label
                    activated: modelData.state > 0
                    activeColor: modelData.state === 1 ? "#a9faa4" : modelData.state === 2 ? "#ffcf00" : "#ff0000"
                }
            }
        }
        EmipInstruments {
            Layout.fillWidth: true
            Layout.fillHeight: true
            telemetry: panel.telemetry
            config: panel.config
        }
    }
}
