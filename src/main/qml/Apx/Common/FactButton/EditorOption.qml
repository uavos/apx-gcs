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
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQml
import QtQuick.Layouts


import "../Button"

ComboBox {
    id: editor

    popup.z: 1000

    spacing: 0
    topPadding: 0
    bottomPadding: 0

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    property real paddingScale: 0.8

    leftPadding: padding + (!editor.mirrored || !indicator || !indicator.visible ? 0 : indicator.width*paddingScale + spacing)
    rightPadding: padding + (editor.mirrored || !indicator || !indicator.visible ? 0 : indicator.width*paddingScale + spacing)

    flat: true

    font: apx.font_narrow(factButton.valueSize)

    background.implicitWidth: contentItem.implicitWidth

    model: fact.enumStrings

    Component.onCompleted: updateIndex()
    onActivated: (index) => {
        fact.setValue(textAt(index))
        if(editor.activeFocus)
            factButton.forceActiveFocus();
    }
    property string value: fact.text
    onValueChanged: updateIndex()
    onModelChanged: updateIndex()

    Connections {
        target: listView
        function onMovementStarted() {
            if(factButton.popup)
                factButton.popup.close()
        }
    }
    delegate: ItemDelegate {
        text: modelData
        width: popup.width
        highlighted: text === editor.value
        font: editor.font
        Component.onCompleted: popup.width=Math.max(popup.width, implicitWidth)
    }

    contentItem: Text {
        leftPadding: height/4
        rightPadding: 0 

        text: editor.value
        font: editor.font
        color: editor.enabled ? editor.Material.foreground : editor.Material.hintTextColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideMiddle
    }

    function updateIndex()
    {
        editor.currentIndex=fact.enumStrings.indexOf(editor.value)
        //console.log(currentIndex,value,count,find(value))
    }
}
