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
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import "."

import Apx.Common 1.0

StatusFlag {
    id: control

    property string title: fact.name.toUpperCase()
    property real value: fact.value

    property real precision: 0

    blinking: failure
    warning: false
    failure: false

    text: value.toFixed(precision)

    width: implicitWidth

    type_default: CleanText.Clean
    type_warning: CleanText.Normal
    show: true

    bold: false

    blinkingFG: false

    prefixItems: [
        Text {
            id: textItem
            Layout.fillHeight: true
            verticalAlignment: Text.AlignTop
            text: control.title
            font: apx.font_narrow(control.height*0.7,true) //Math.max(Math.min(12,control.height), control.height*0.6))
            color: "#80DEEA"
        }
    ]
}

