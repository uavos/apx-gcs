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
// switched by its button in the panel buttons row.
// The panel and its button are there only while the plugin is enabled
AppPlugin {
    id: plugin

    sourceComponent: TrafficView { }

    uiComponent: "main"
    onConfigure: trafficSync(false)

    // The main layout copies the plugin properties into its buttons model,
    // so the names here must not clash with its own functions (show, add)

    // the panel follows the plugin state (startup: the layout restores the saved panel)
    Connections {
        target: fact
        function onActiveChanged(){ plugin.trafficSync(false) }
    }
    // switched on by the user: bring the panel to front
    Connections {
        target: fact ? fact.child("use") : null
        function onValueChanged(){ plugin.trafficSync(true) }
    }

    // added to the instruments layout and listed in its buttons row
    property bool trafficListed: false

    // enabled: add (once) and bring the panel to front (front), disabled: take it out of the row
    function trafficSync(front)
    {
        if(!ui.main) return // the main layout is not loaded yet, called again on configure
        var inst = trafficFindInstruments(ui.main, 3)
        if(fact.active){
            if(!trafficListed){
                trafficListed = true
                ui.main.add(plugin, GroundControl.Layout.Instrument)
            }
            if(front && inst && inst.show) inst.show(plugin)
            return
        }
        if(!trafficListed) return
        trafficListed = false
        if(inst) trafficRemoveFrom(inst)
    }
    // the instruments layout has no remove: take the entry out of the buttons model
    // (the model of its ListView) and switch its StackView to another panel.
    // The layout keeps the plugin in its plugins array, the next add appends a new entry
    function trafficRemoveFrom(inst)
    {
        var model = null
        var stack = null
        for(var i=0;i<inst.children.length;++i){
            var c = inst.children[i]
            if(c.delegate!==undefined && c.model!==undefined) model = c.model
            else if(c.replace!==undefined && c.currentItem!==undefined) stack = c
        }
        if(!model || !inst.plugins) return
        for(var j=0;j<model.count;++j){
            if(inst.plugins[model.get(j).idx]!==plugin) continue
            model.remove(j)
            break
        }
        if(stack && stack.currentItem===plugin && model.count>0 && inst.show)
            inst.show(inst.plugins[model.get(0).idx])
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
