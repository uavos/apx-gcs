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
import QtPositioning 5.6

import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

import APX.Vehicles 1.0 as APX
import APX.Mission 1.0

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Menu 1.0
//import Apx.Map 1.0

RowLayout {

    readonly property APX.Vehicle vehicle: apx.vehicles.current

    readonly property Mission mission: vehicle.mission

    height: missionButton.height
    //spacing: 10*ui.scale
    //property int itemSize: Math.max(10,missionButton.height)
    //property int iconFontSize: itemSize*0.8
    //property int titleFontSize: itemSize*0.8
    TextButton {
        id: missionButton
        minimumWidth: height*3
        color: mission.modified?"#FFF59D":"#A5D6A7"
        Material.theme: Material.Light
        onClicked: mission.trigger()
        text: (mission.text)
              +"\n"+(mission.empty?"":mission.wp.descr)
        textScale: 0.45
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
        fact: mission.tools.save
        showText: false
        visible: (!mission.saved) && (!mission.empty)
    }
    ActionButton {
        fact: mission.tools.load
        showText: false
        visible: (mission.empty)
    }
}
