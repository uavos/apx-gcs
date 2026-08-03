import QtQuick
import QtQuick.Layouts

Panel {
    required property var page

    title: qsTr("MANUAL")
    enabled: page.modeText === "manual"
    opacity: enabled ? 1.0 : 0.5
    contentSpacing: 2

    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: qsTr("Step"); color: "#888888"; font.pixelSize: 12
            Layout.preferredWidth: 16 + 46 + 4
        }
        StepSpin {
            id: stepSpin
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: "Az"; color: "#888888"; font.pixelSize: 12
            Layout.preferredWidth: 16
        }
        Text {
            text: page.cmdYaw.toFixed(1) + "°"
            color: "#00ffff"; font.pixelSize: 13; font.bold: true
            Layout.preferredWidth: 46
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "-"
            onClicked: page.nudgeCmd("yaw", -stepSpin.realStep)
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "+"
            onClicked: page.nudgeCmd("yaw", stepSpin.realStep)
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: "El"; color: "#888888"; font.pixelSize: 12
            Layout.preferredWidth: 16
        }
        Text {
            text: page.cmdPitch.toFixed(1) + "°"
            color: "#00ffff"; font.pixelSize: 13; font.bold: true
            Layout.preferredWidth: 46
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "-"
            onClicked: page.nudgeCmd("pitch", -stepSpin.realStep)
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "+"
            onClicked: page.nudgeCmd("pitch", stepSpin.realStep)
        }
    }
    ToolBtn {
        Layout.fillWidth: true
        text: qsTr("Go to Max")
        enabled: page.maxRssi > 0
        onClicked: page.goToMax()
    }
}
