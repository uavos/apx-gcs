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
import QtPositioning

import QtQuick.Controls
import QtQuick.Controls.Material

import APX.Fleet as APX
import APX.Mission

import Apx.Common
import Apx.Controls
import Apx.Menu

Row {

    readonly property APX.Unit unit: apx.fleet.current

    readonly property Mission mission: unit.mission

    height: missionButton.height
    spacing: Style.spacing

    TextButton {
        id: missionButton
        minimumWidth: height*3
        color: mission.modified?"#FFF59D":"#A5D6A7"
        Material.theme: Material.Light
        onClicked: mission.trigger()
        text: (mission.text)
              +"\n"+(mission.empty?"":mission.wp.descr)
        textScale: 0.5
        lineHeight: 0.75
    }
    ActionButton {
        fact: mission.request
        showText: false
        visible: (!mission.synced)
    }
    ActionButton {
        fact: mission.upload
        showText: false
        visible: (!mission.synced) && (!mission.empty)
    }
    ActionButton {
        fact: mission.tools.share.imp
        showText: false
        visible: (mission.empty)
    }
}
