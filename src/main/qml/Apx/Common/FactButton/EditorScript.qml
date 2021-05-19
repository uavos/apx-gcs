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
        fact.script.setSource(editedTitle, editedText)
    }

    property alias editedTitle: scriptName.text
    property alias editedText: textArea.text

    contentItem: ColumnLayout {
        implicitHeight: Style.buttonSize*25
        implicitWidth: Style.buttonSize*25
    
        TextField {
            id: scriptName
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            placeholderText: qsTr("Script title")
            selectByMouse: true
            font: apx.font_narrow(Style.fontSize)
            text: fact.script.title
            background: Rectangle {
                color: "#353637"
                border.width: 0
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Style.spacing
            TextArea {
                id: textArea
                selectByMouse: true
                selectByKeyboard: true
                text: value
                textFormat: TextEdit.PlainText
                wrapMode: Text.NoWrap
                font: apx.font_fixed(Style.buttonSize*0.4)
                background: Rectangle {
                    border.color: "#aaa"
                    color: "#000"
                }
            }
        }
    }

}
