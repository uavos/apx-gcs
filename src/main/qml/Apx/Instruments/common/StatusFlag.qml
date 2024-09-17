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
import "."

BlinkingText {
    id: control

    text: fact.title.toUpperCase()

    property real status: fact.value
    property real status_warning: 2
    property real status_show: status_warning
    property real status_failure: status_warning+1

    property int status_reset: -1

    property int type_default: CleanText.White
    property int type_warning: CleanText.Yellow

    width: height*2.2

    show: ui.test || status >= status_show
    blinking: failure

    property bool warning: status >= status_warning
    property bool failure: status >= status_failure


    type: failure
          ? CleanText.Red
          : warning
            ? type_warning
            : type_default

    visible: ui.test || show

    MouseArea {
        anchors.fill: parent
        enabled: status_reset >= 0 && show
        onClicked: fact.value = status_reset
    }
}

