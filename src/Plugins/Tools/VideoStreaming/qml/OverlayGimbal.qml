/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.12
import QtQuick.Layouts 1.12

ColumnLayout {
    id: control

    property var m_pitch: mandala.findChild(plugin.tune.overlay.gimbal_pitch_var.text)
    property var m_yaw: mandala.findChild(plugin.tune.overlay.gimbal_yaw_var.text)

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
