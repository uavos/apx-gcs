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
import QtQuick.Controls 2.12

import APX.Facts 1.0

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
    property bool selected: false
    property int id: 0

    readonly property color color : {

        var cImportant = (source==AppNotify.FromVehicle)?"#aff":"#afa"
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
        focus: false
        color: selected ? "#0977e3" : control.color
        font: apx.font(lineSize,control.bold)
        wrapMode: Text.WrapAnywhere
        text: control.text
        textFormat: html?Text.RichText:Text.AutoText

        Keys.onUpPressed: listView.Keys.upPressed(event)
        Keys.onDownPressed: listView.Keys.downPressed(event)

        MouseArea {
            acceptedButtons: Qt.LeftButton
            anchors.fill: parent
            onClicked: {
                if (mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) {
                    listView.currentIndex = index
                    listView.currentItem.id = index
                    listView.selectedItems.push(listView.currentItem)
                    for (var i = listView.selectedItems.length - 1; i >= 1 &&
                        listView.selectedItems[i].id < listView.selectedItems[i-1].id; i--) {
                        var tmp = listView.selectedItems[i]
                        listView.selectedItems[i] = listView.selectedItems[i-1]
                        listView.selectedItems[i-1]=tmp
                    }
                    selected=true
                    forceActiveFocus()
                }
                if (mouse.modifiers == Qt.NoModifier) {
                    listView.footerItem.setFocus()
                }
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
}
