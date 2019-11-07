import QtQuick 2.12
import QtQuick.Layouts 1.12

ColumnLayout {
    id: control

    property real pitch: m.roll.value
    property real yaw: m.yaw.value

    spacing: 5

    OverlayGimbalAxis {
        Layout.fillWidth: true
        value: pitch
        type: OverlayGimbalAxis.AxisType.Down
    }

    OverlayGimbalAxis {
        Layout.fillWidth: true
        value: yaw
        type: OverlayGimbalAxis.AxisType.Full
    }

}
