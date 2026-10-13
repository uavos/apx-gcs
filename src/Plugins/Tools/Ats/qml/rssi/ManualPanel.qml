import QtQuick
import QtQuick.Layouts

import Apx.Common

Panel {
    required property var page

    title: qsTr("MANUAL")
    enabled: page.gcs !== null && page.mode === ats_mode_manual
    opacity: enabled ? 1.0 : 0.5
    contentSpacing: 2

    NudgeRow {
        label: "Az"
        value: page.cmdYaw
        onNudge: (d) => page.nudgeCmd("yaw", d)
    }
    NudgeRow {
        label: "El"
        value: page.cmdPitch
        onNudge: (d) => page.nudgeCmd("pitch", d)
    }
    ToolBtn {
        Layout.fillWidth: true
        text: qsTr("Go to Max")
        enabled: page.maxRssi > 0
        onClicked: page.goToMax()
    }
}
