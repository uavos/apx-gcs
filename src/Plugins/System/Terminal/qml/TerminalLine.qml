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
import QtQuick.Controls

import APX.Facts

RowLayout {
    id: control

    //prefs
    spacing: lineSpace

    //model data
    property string text
    property string subsystem
    property string source
    property int type
    property int options
    property var fact
    property int timestamp

    readonly property color color: {

        var cImportant = (source==AppNotify.FromUnit)?"#aff":"#afa"
        var cInfo = (source==AppNotify.FromInput)?"#ccc":"#aaa"

        switch(type){
        default:
        case AppNotify.Info: return cInfo
        case AppNotify.Important: return cImportant
        case AppNotify.Warning: return "#ff8"
        case AppNotify.Error: return "#f88"
        }
    }
    readonly property bool html: text.startsWith("<html>")
    readonly property bool bold: {
        if(source==AppNotify.FromInput) return true
        if(type==AppNotify.Info) return false
        return true
    }

    Label {
        Layout.fillWidth: true
        color: control.color
        font: apx.font(lineSize,control.bold)
        wrapMode: Text.WrapAnywhere
        text: control.text
        textFormat: html?Text.RichText:Text.AutoText
        background: Rectangle {
            anchors.fill: parent
            color: "#ffffff"
            opacity: 0.15
            visible: menu.visible
        }
        
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: (mouse) => {
                if (mouse.button === Qt.RightButton)
                    menu.popup()
            }
            onPressAndHold: (mouse) => {
                if (mouse.source === Qt.MouseEventNotSynthesized)
                    menu.popup()
            }
            Menu {
                id: menu
                width: 105 * Math.max(0.75,1*ui.scale)

                TerminalMenuItem { 
                    text: qsTr("Copy")
                    onTriggered: control.copyMessage()
                }
                TerminalMenuItem { 
                    text: qsTr("Copy all")
                    onTriggered: control.copyAllMessages()
                }
            }
            TextEdit {
                id: textEdit
                visible: false
            }
        }
    }
    Label {
        Layout.alignment: Qt.AlignRight|Qt.AlignVCenter
        visible: text //&& control.x==0
        text: control.subsystem
        color: "#aaa"
        font: apx.font_narrow(lineSize*0.9)

        background: Rectangle {
            border.width: 0
            color: "#223"
            radius: height/10
        }
    }
    function copyMessage() {
        textEdit.text = control.text
        copy2Clipboard()
    }
    function copyAllMessages() {
        for (let i = 0; i < listView.count; i++) {
            if (!listView.itemAtIndex(i))
                continue
            if(i > 0)
                textEdit.text += "\n"
            textEdit.text += listView.itemAtIndex(i).text
        }
        copy2Clipboard()
    }
    function copy2Clipboard() {
        textEdit.selectAll()
        textEdit.copy()
        textEdit.text = "";
    }
}
