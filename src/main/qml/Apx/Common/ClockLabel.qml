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
import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Apx.Common 1.0

Item {
    id: control
    implicitWidth: textItem.implicitWidth

    property alias color: textItem.color
    Text {
        id: textItem
        anchors.fill: parent
        font: apx.font_narrow(control.height)
        verticalAlignment: Text.AlignVCenter
        color: "#fff"
        Timer {
            interval: 500; running: true; repeat: true;
            onTriggered: textItem.text=apx.timeString()
        }
    }
    //Rectangle { anchors.fill: control; color: "#80ffffff" }
}
