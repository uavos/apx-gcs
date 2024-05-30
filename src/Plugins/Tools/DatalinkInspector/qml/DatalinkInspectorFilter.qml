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

import Apx.Common

ListView {
    id: listView

    clip: true

    implicitWidth: contentItem.childrenRect.width

    signal filter(var uid, var exclude)

    spacing: Style.spacing/2

    model: plugin_fact.uidModel

    delegate: DatalinkInspectorItem {
        text: model.blocks.join(' ')
        size: Style.buttonSize*0.4
        //itemColor: model.color
        MouseArea {
            anchors.fill: parent
            onClicked: {
                invert=!invert
                filter(modelData[0], invert)
            }
        }
    }
}
