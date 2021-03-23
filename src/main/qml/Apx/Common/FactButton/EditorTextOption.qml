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
import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQml 2.12

EditorOption {
    id: editor

    editable: true

    paddingScale: 1
    
    contentItem: TextInput {
        id: textInput
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignRight
        font.family: font_condenced
        font.pixelSize: control.valueSize
        color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
        text: fact.text
        selectByMouse: true
        onEditingFinished: {
            fact.setValue(text);
            control.focusFree();
        }
        onActiveFocusChanged: if(activeFocus)selectAll();
    }
    background: //Item {}
    Rectangle {
        z: parent.z-1
        visible: fact.enabled
        anchors.right: contentItem.right
        anchors.verticalCenter: contentItem.verticalCenter
        anchors.rightMargin: -10
        width: contentItem.contentWidth-anchors.rightMargin*2
        height: contentItem.height
        radius: 3
        color: "#000"
        border.width: 0
        opacity: 0.3
    }

    Connections {
        target: control
        function onFocusRequested(){ checkFocusRequest() }
    }
    Component.onCompleted: checkFocusRequest()
    function checkFocusRequest()
    {
        if(!control.bFocusRequest)return
        control.bFocusRequest=false
        editor.forceActiveFocus()
    }

}
