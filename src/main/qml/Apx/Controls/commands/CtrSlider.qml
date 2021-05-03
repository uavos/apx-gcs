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
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

RowLayout {
    id: control
    property var fact
    property alias title: titleItem.text
    property alias from: ctr.from
    property alias to: ctr.to
    property alias stepSize: ctr.stepSize

    property real size: buttonHeight
    property real titleWidth: size*1.5
    property real valueWidth: titleWidth

    property string toolTip: info

    property string info: fact.title+" ("+fact.descr+")"

    spacing: 0

    TextButton {
        id: titleItem
        Layout.minimumWidth: titleWidth
        size: control.size
        color: highlighted?undefined:"#000"
        highlighted: fact.value!==0
        onTriggered: fact.value=0
        toolTip: control.toolTip
    }

    Slider {
        id: ctr
        Layout.fillWidth: true
        Layout.maximumHeight: size

        property real v: fact.value

        from: -1
        to: 1
        stepSize: 0.01

        value: v

        snapMode: Slider.SnapOnRelease

        onMoved: timer.start()
        onVChanged: value=v

        Timer {
            id: timer
            interval: 100
            onTriggered: fact.value=ctr.value
        }
    }

    Label {
        Layout.minimumWidth: valueWidth
        font: apx.font_narrow(control.size*0.8)
        text: fact.value.toFixed(2)
        horizontalAlignment: Text.AlignRight
    }
}
