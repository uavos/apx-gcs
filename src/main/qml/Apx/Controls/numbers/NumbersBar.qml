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

Flow {
    id: control
    spacing: itemSize*0.2
    clip: true

    property bool showEditButton: true
    property alias settingsName: numbersModel.settingsName
    property alias defaults: numbersModel.defaults
    property alias model: numbersModel

    property real itemSize: Style.buttonSize


    Loader {
        active: showEditButton
        visible: active
        sourceComponent: IconButton {
            size: numbersModel.itemHeight
            iconName: "note-plus-outline"//"plus-circle"
            toolTip: qsTr("Edit display values")
            onTriggered: numbersModel.edit()
            opacity: ui.effects?(hovered?1:0.5):1
        }
    }

    Repeater {
        model: numbersModel
    }


    NumbersModel {
        id: numbersModel
        itemHeight: itemSize
        light: true
    }
}
