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

import Qt.labs.settings 1.0
import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import Apx.Common 1.0
import Apx.Menu 1.0
import APX.Facts 1.0

Dialog {
    id: editor

    property QtObject fact

    property string value: fact? fact.script.source:""

    visible: true
    title: qsTr("Script editor")
    standardButtons: Dialog.Ok | Dialog.Apply | Dialog.Cancel
    onClosed: destroy()
    modal: false
    closePolicy: Popup.NoAutoClose

    x: Math.max(0, (parent.width-width)/2)
    y: Math.max(0, (parent.height-height)/2)

    padding: 0

    onApplied: compile()
    onAccepted: compile()

    onFactChanged: {
        if(!fact) reject()
    }

    function compile()
    {
        fact.script.setSource(fact.script.title, editedText)
    }

    property alias editedText: textArea.text

    //contentWidth: layout.implicitWidth
    contentHeight: layout.implicitHeight
    contentItem:
    ColumnLayout {
        id: layout
        ScrollView {
            Layout.topMargin: header.height
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: textArea
                selectByMouse: true
                selectByKeyboard: true
                text: value
                textFormat: TextEdit.PlainText
                wrapMode: Text.NoWrap
                font.family: font_fixed
                Material.background: "#000"
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.bottomMargin: footer.height
        }
    }

}
