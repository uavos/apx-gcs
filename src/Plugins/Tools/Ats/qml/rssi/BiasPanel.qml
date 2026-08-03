import QtQuick
import QtQuick.Layouts

Panel {
    required property var page

    title: qsTr("BIAS")
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
            text: page.biasYawFact ? page.biasYawFact.value.toFixed(1) + "°" : "--"
            color: "#00ffff"; font.pixelSize: 13; font.bold: true
            Layout.preferredWidth: 46
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "-"
            onClicked: page.nudgeBias("yaw", -stepSpin.realStep)
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "+"
            onClicked: page.nudgeBias("yaw", stepSpin.realStep)
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
            text: page.biasPitchFact ? page.biasPitchFact.value.toFixed(1) + "°" : "--"
            color: "#00ffff"; font.pixelSize: 13; font.bold: true
            Layout.preferredWidth: 46
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "-"
            onClicked: page.nudgeBias("pitch", -stepSpin.realStep)
        }
        ToolBtn {
            Layout.fillWidth: true
            text: "+"
            onClicked: page.nudgeBias("pitch", stepSpin.realStep)
        }
    }
    ToolBtn {
        Layout.fillWidth: true
        text: qsTr("Bias from Max")
        enabled: page.maxRssi > 0
        onClicked: page.setBiasFromMax()
    }
}
