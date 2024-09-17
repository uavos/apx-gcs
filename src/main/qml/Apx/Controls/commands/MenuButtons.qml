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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Common
import Apx.Menu

ListView {
    id: listView
    orientation: ListView.Horizontal
    model: model.model
    spacing: buttonSpacing

    implicitHeight: buttonHeight
    implicitWidth: 100

    delegate: IconButton {
        size: listView.height
        toolTip: modelData.descr?modelData.descr:modelData.title
        iconName: modelData.icon
        highlighted: modelData.active
        color: Qt.darker(Material.color(Material.Blue),2.2)
        onTriggered: {
            Menu.show(modelData,{},root)
        }
    }

    MenuModel {
        id: model
    }
}
