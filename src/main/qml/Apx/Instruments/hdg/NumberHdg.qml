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
import QtQuick 2.2
import "../common"

Item {
    id: numValue
    property variant mfield
    property real precision: 0

    property string label: mfield?mfield.name:""
    property real value: mfield?mfield.value:0
    property string text: value.toFixed(precision)
    property string toolTip: mfield?mfield.title:""
    property string labelColor: "#aaa"
    property string valueColor: "yellow"

    property real labelScale: 0.7
    property real labelMargin: 2
    property string labelFont: "Monospace"
    property string valueFont: "FreeMono Bold"

    property bool blinking: (alarm)?true:false
    property color color: alarm?"red":warning?"#40ffff30":"transparent"

    property bool warning: false
    property bool alarm: false
    property real diff: 0

    //mandala alarms
    onWarningChanged: showWarningTimer.start();
    onAlarmChanged: showAlarmTimer.start();
    Timer {
        id: showWarningTimer
        interval: 500; running: false; repeat: false
        onTriggered: {
            if(warning && (!alarm)){
                apx.vehicles.current.warnings.warning(mfield?mfield.title:label);
            }
        }
    }
    Timer {
        id: showAlarmTimer
        interval: 100; running: false; repeat: false
        onTriggered: {
            if(alarm){
                showWarningTimer.stop();
                apx.vehicles.current.warnings.error(mfield?mfield.title:label);
            }else if(warning) showWarningTimer.start();
        }
    }


    //diff monitor
    property real value_s: value
    onValueChanged: {
        diff=value_s===0?0:Math.abs(value-value_s);
        value_s=value;
    }

    width: numValue_label.width+numValue_value.width+1
    property bool blink: true
    onBlinkingChanged: blink=true

    SequentialAnimation on visible {
        running: blinking && (numValue.color!="transparent")
        loops: Animation.Infinite
        PropertyAction { target: numValue; property: "blink"; value: true }
        PauseAnimation { duration: 300 }
        PropertyAction { target: numValue; property: "blink"; value: false }
        PauseAnimation { duration: 300 }
    }

    Rectangle {
        id: numBG
        anchors.fill: parent
        anchors.bottomMargin: 2
        radius: 3
        color: numValue.color
        opacity: numValue.blink?1:0
        Behavior on opacity { enabled: ui.smooth; PropertyAnimation {duration: 100} }
    }

    Text {
        id: numValue_label
        //smooth: ui.antialiasing
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignTop
        //text:  "<pre>"+label+"</pre>"
        text:  label
        font.pixelSize: parent.height*labelScale
        font.family: labelFont
        color: labelColor
    }
    Text {
        id: numValue_value
        anchors.left: numValue_label.right
        anchors.leftMargin: numValue.labelMargin
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: numValue.text
        font.pixelSize: parent.height
        font.family: valueFont
        //font.bold: (valueFont !== font_narrow)
        color: valueColor
    }
    ToolTipArea {
        text: numValue.toolTip
    }
}


