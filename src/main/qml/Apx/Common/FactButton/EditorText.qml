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
import QtQuick.Layouts
import QtQml

import Apx.Common

Rectangle {
    id: editor
    implicitHeight: textInput.contentHeight+Style.spacing
    implicitWidth: Math.max(textInput.contentWidth+Style.spacing,height*2)

    radius: height/10
    color: "#50000000"
    border.width: 0

    TextInput {
        id: textInput
        anchors.centerIn: parent
        clip: true
        focus: true

        width: editor.width-4

        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter

        font: apx.font_narrow(factButton.valueSize)

        color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
        text: fact.text

        activeFocusOnTab: true
        selectByMouse: true

        onEditingFinished: {
            fact.setValue(text);
            factButton.forceActiveFocus();
        }
        onActiveFocusChanged: {
            if(activeFocus)selectAll();
        }
    }
}
