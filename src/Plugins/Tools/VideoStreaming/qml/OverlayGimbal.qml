import QtQuick 2.12
import QtQuick.Layouts 1.12

ColumnLayout {
    id: control

    property var m_pitch: m[plugin.tune.overlay.gimbal_pitch_var.text]
    property var m_yaw: m[plugin.tune.overlay.gimbal_yaw_var.text]

    spacing: width/20

    Loader {
        Layout.fillWidth: true
        active: m_pitch?true:false
        visible: active
        sourceComponent: OverlayGimbalAxis {
            value: m_pitch.value
            type: OverlayGimbalAxis.AxisType.Down
        }
    }

    Loader {
        Layout.fillWidth: true
        active: m_yaw?true:false
        visible: active
        sourceComponent: OverlayGimbalAxis {
            value: m_yaw.value
            type: OverlayGimbalAxis.AxisType.Full
        }
    }

}
