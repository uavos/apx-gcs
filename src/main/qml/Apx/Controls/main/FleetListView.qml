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
import QtLocation
import QtPositioning

import APX.Fleet
import Apx.Common

Item {
    id: control
    implicitHeight: fleetList.height
    implicitWidth: fleetList.width
    Layout.minimumWidth: height*2
    ListView {
        id: fleetList
        model: apx.fleet.model
        implicitHeight: contentItem.childrenRect.height
        implicitWidth: Math.min(contentItem.childrenRect.width,parent.width)
        orientation: ListView.Horizontal
        snapMode: ListView.SnapToItem
        delegate: UnitButton {
            enabled: true
            unit: modelData
            Connections {
                target: unit
                function onSelected(){
                    fleetList.positionViewAtIndex(index, ListView.Beginning)
                }
            }
        }

        spacing: Style.spacing*2
        clip: true

        Component {
            id: unitInfoDelegate
            UnitButton {
                enabled: true
                unit: modelData
            }
        }
    }
    //Rectangle { anchors.fill: control; color: "#80ffffff" }
}
