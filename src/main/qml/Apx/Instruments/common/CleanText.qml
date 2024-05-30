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
import QtQuick
import QtQuick.Layouts
import "."

Item {
    id: control
    property var fact

    enum Type {
        Normal,
        Clean,
        White,
        Green,
        Yellow,
        Red,
        Blue,
        Black
    }

    property int type: CleanText.Normal

    property string text: fact.text
    property string prefix
    property string toolTip: fact.title+": "+fact.text

    property string displayText: (control.prefix ? control.prefix + " " + control.text : control.text).toUpperCase()

    property bool bold: true

    property bool show: true
    property bool frame: false

    property color bgColor: {
        switch(type){
            default: return "#80000000"
            case CleanText.Clean: return "transparent"
            case CleanText.White: return "#ddd"
            case CleanText.Green: return "green"
            case CleanText.Yellow: return "yellow"
            case CleanText.Red: return "red"
            case CleanText.Blue: return "blue"
            case CleanText.Black: return "black"
        }
    }
    property color textColor: {
        if(frame)
            return "white"
        switch(type){
            default: return "white"
            case CleanText.Yellow:
            case CleanText.White:
                return "black"
        }
    }

    Component.onCompleted: {
        if(!fact)
            console.info(control.text, parent, parent.parent)
    }

    property bool hide_bg: hide_fg
    property bool hide_fg: !(show || ui.test)

    Rectangle {
        anchors.fill: parent
        visible: !frame
        color: bgColor
        opacity: hide_bg?0:1
        radius: height/10
    }

    implicitWidth: _body.implicitWidth + 4
    implicitHeight: Math.max(8, textItem.implicitHeight)

    property alias prefixItems: _prefixItems.children
    RowLayout {
        id: _body
        anchors.fill: parent
        opacity: hide_fg?0:1
        spacing: 0
        RowLayout {
            id: _prefixItems
            Layout.fillWidth: false
            Layout.fillHeight: true
            spacing: 0
        }
        Text {
            id: textItem
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: control.displayText
            font: apx.font_narrow(control.height, control.bold)
            color: textColor
        }
    }
    Rectangle {
        anchors.fill: _body
        anchors.leftMargin: _prefixItems.width
        visible: frame
        color: "transparent"
        radius: height/10
        border.width: 1
        border.color: bgColor
    }

    ToolTipArea {
        text: control.toolTip
    }
}

