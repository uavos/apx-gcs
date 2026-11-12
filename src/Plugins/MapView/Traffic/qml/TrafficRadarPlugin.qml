/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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

import Apx.Common
import Apx.Application

// Traffic radar: a widget in the main layout (bottom right, like Signals and the terrain
// elevation chart) switched by its button in the widgets bar.
// The button is there only while the plugin is enabled
AppPlugin {
    id: plugin

    title: qsTr("Radar")
    descr: qsTr("Traffic radar")
    icon: "radar"

    enabled: fact ? fact.active : true
    onEnabledChanged: if(!enabled) active = false

    sourceComponent: TrafficRadar { }

    uiComponent: "main"
    onConfigure: {
        ui.main.add(plugin, GroundControl.Layout.Main)
    }
}
