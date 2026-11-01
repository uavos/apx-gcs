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

import Apx.Application

// Traffic list: an instruments panel next to the Terminal,
// switched by its button in the panel buttons row
AppPlugin {
    id: plugin

    sourceComponent: TrafficView { }

    uiComponent: "main"
    onConfigure: ui.main.add(plugin, GroundControl.Layout.Instrument)

    // bring the panel to front when the plugin is switched on.
    // The main layout copies the plugin properties into its buttons model,
    // so the names here must not clash with its own functions (show, add)
    Connections {
        target: fact
        function onActiveChanged(){ if(fact.active) plugin.trafficBringToFront() }
    }
    function trafficBringToFront()
    {
        var inst = trafficFindInstruments(ui.main, 3)
        if(inst && inst.show) inst.show(plugin)
    }
    // the instruments stack of the main layout (InstrumentsLayout.instrumentsItem)
    function trafficFindInstruments(item, depth)
    {
        if(!item || depth<0) return null
        if(item.instrumentsItem) return item.instrumentsItem
        for(var i=0;i<item.children.length;++i){
            var r=trafficFindInstruments(item.children[i], depth-1)
            if(r) return r
        }
        return null
    }
}
