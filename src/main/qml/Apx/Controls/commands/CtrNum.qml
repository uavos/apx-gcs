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

    property real min: -100
    property real max: 100
    property real stepSize: 1
    property int precision: 0
    property real mult: 1

    property int size: buttonHeight
    property int titleWidth: size*1.8
    property int valueWidth: size

    spacing: 4

    property real value: fact.value*mult

    TextButton {
        id: titleItem
        Layout.minimumWidth: titleWidth
        size: control.size
        color: highlighted?undefined:"#000"
        highlighted: value!==0
        onTriggered: fact.value=0
        toolTip: qsTr("Reset")+" "+fact.descr
    }

    Label {
        Layout.minimumWidth: valueWidth
        font.family: font_narrow
        font.pixelSize: control.size*0.8
        text: value.toFixed(precision)
        horizontalAlignment: Text.AlignHCenter
        Rectangle {
            x: -parent.x
            y: -parent.y
            width: control.width-1
            height: control.height-1
            border.width: 1
            border.color: "#222"
            color: "transparent"
            radius: 6
        }
    }

    TextButton {
        size: control.size
        text: "-"
        enabled: value>min
        onTriggered: adjust(-stepSize)
        onPressAndHold: {
            timer.step=-stepSize
            timer.start()
        }
        onReleased: timer.stop()
        toolTip: qsTr("Decrease")+" "+fact.descr
    }
    TextButton {
        size: control.size
        text: "+"
        enabled: value<max
        onTriggered: adjust(stepSize)
        onPressAndHold: {
            timer.step=stepSize
            timer.start()
        }
        onReleased: timer.stop()
        toolTip: qsTr("Increase")+" "+fact.descr
    }
    Timer {
        id: timer
        property real step: 0
        interval: 100
        repeat: true
        onTriggered: if(!adjust(step))stop()
    }

    function adjust(v)
    {
        var x=value+v
        if(x>=max)x=max
        else if(x<min)x=min
        else {
            fact.value=x/mult
            return 1
        }
        fact.value=x/mult
        return 0
    }
}
