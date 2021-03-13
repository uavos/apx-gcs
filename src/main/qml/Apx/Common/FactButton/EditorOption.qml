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
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.2
import QtQml 2.12
import QtQuick.Layouts 1.3


import "../Button"

ComboBox {
    id: editor

    spacing: 0
    topPadding: 0
    bottomPadding: 0

    topInset: 0
    bottomInset: 0

    padding: 0
    leftPadding: padding + (!control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width*0.8 + spacing)
    rightPadding: padding + (control.mirrored || !indicator || !indicator.visible ? 0 : indicator.width*0.8 + spacing)

    flat: true

    font.family: font_condenced
    font.pixelSize: control.valueSize

    background.implicitWidth: contentItem.implicitWidth

    model: fact.enumStrings

    Component.onCompleted: updateIndex()
    onActivated: {
        fact.setValue(textAt(index))
        parent.forceActiveFocus();
    }
    property string value: fact.text
    onValueChanged: updateIndex()
    onModelChanged: updateIndex()

    Connections {
        target: listView
        function onMovementStarted() {
            control.popup.close()
        }
    }
    delegate: ItemDelegate {
        text: modelData
        width: popup.width
        highlighted: text === fact.text
        font: editor.font
        Component.onCompleted: popup.width=Math.max(popup.width, implicitWidth)
    }

    contentItem: Text {
        leftPadding: height/4
        rightPadding: 0 //control.indicator.width + control.spacing

        text: editor.displayText
        font: editor.font
        color: editor.enabled ? editor.Material.foreground : editor.Material.hintTextColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideMiddle
    }

    function updateIndex()
    {
        //currentIndex=find(value)
        editor.currentIndex=fact.enumStrings.indexOf(control.value)
        //console.log(currentIndex,value,count,find(value))
    }
}
