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
import QtCore

import Apx.Application
import APX.Facts

/*
    EMIP instrument panel (engine, fuel, annunciators).
    Bindings and limits: bundled defaults in EmipConfig.qml, overridden by the user file
    <Documents>/UAVOS/emip/emip.json (same keys).
    The panel is off by default: Tools, System features, EMIP, Enabled.
*/

AppPlugin {
    id: plugin

    readonly property bool isEmipPanel: true
    title: qsTr("EMIP")
    descr: qsTr("Engine and fuel instruments")
    icon: "gauge"

    unloadOnHide: true
    visible: false // shown by the instruments layout once registered
    uiComponent: "main"

    // the only setting of the plugin
    Fact {
        id: f_tool
        name: "emip"
        title: qsTr("EMIP")
        descr: qsTr("Engine monitoring panel settings")
        icon: "gauge"
        flags: Fact.Group
        section: qsTr("System features")
        Component.onCompleted: parentFact = apx.tools
        Fact {
            id: f_enabled
            name: "enabled"
            title: qsTr("Enabled")
            descr: qsTr("Show the panel and the FP page")
            flags: Fact.Bool | Fact.PersistentValue
            defaultValue: false
        }
    }
    readonly property bool panelEnabled: f_enabled.value ? true : false

    // the panel and the FP page are registered while enabled
    property bool registered: false
    onConfigure: update()
    onPanelEnabledChanged: update()
    function update()
    {
        if(!ui.main || registered === panelEnabled)
            return
        registered = panelEnabled
        if(panelEnabled){
            ui.main.add(plugin, GroundControl.Layout.Instrument)
            // fuel pumps page in the Controls panel
            if(typeof(ui.main.addControlPage) === "function")
                ui.main.addControlPage({ "name": "FP", "mode": "", "source": Qt.resolvedUrl("EmipPageFP.qml") })
            return
        }
        var inst = findInstruments(ui.main, 3)
        if(inst)
            removeFrom(inst)
        if(ui.main.controlPages)
            ui.main.controlPages = ui.main.controlPages.filter(function(p){ return p.name !== "FP" })
    }
    // the instruments layout has no remove: take the entry out of the buttons model
    // (the model of its ListView) and switch its StackView to another panel.
    // The layout keeps the plugin in its plugins array, the next add appends a new entry
    function removeFrom(inst)
    {
        var model = null
        var stack = null
        for(var i = 0; i < inst.children.length; ++i){
            var c = inst.children[i]
            if(c.delegate !== undefined && c.model !== undefined) model = c.model
            else if(c.replace !== undefined && c.currentItem !== undefined) stack = c
        }
        if(!model || !inst.plugins) return
        for(var j = 0; j < model.count; ++j){
            if(inst.plugins[model.get(j).idx] !== plugin) continue
            model.remove(j)
            break
        }
        if(stack && stack.currentItem === plugin && model.count > 0 && inst.show)
            inst.show(inst.plugins[model.get(0).idx])
        plugin.visible = false
    }
    // the instruments stack of the main layout (InstrumentsLayout.instrumentsItem)
    function findInstruments(item, depth)
    {
        if(!item || depth < 0) return null
        if(item.instrumentsItem) return item.instrumentsItem
        for(var i = 0; i < item.children.length; ++i){
            var r = findInstruments(item.children[i], depth - 1)
            if(r) return r
        }
        return null
    }

    // user JSON config overrides the bundled defaults (same keys as EmipConfig.qml)
    readonly property string userConfig: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/UAVOS/emip/emip.json"
    EmipConfig { id: instrumentConfig }
    function loadUserConfig()
    {
        var obj = application.loadJson(userConfig)
        if(obj === undefined || obj === null){
            // no file yet: write the defaults as a template, never overwrite an existing file
            console.warn("EMIP: config file not found, using defaults:", userConfig)
            saveDefaultConfig()
            return
        }
        if(typeof(obj) !== "object"){
            console.warn("EMIP: config is not a JSON object, using defaults:", userConfig)
            return
        }
        var n = 0
        for (var key in obj) {
            if(!instrumentConfig.hasOwnProperty(key)){
                console.warn("EMIP: unknown config key:", key)
                continue
            }
            instrumentConfig[key] = obj[key]
            n++
        }
        console.log("EMIP: config loaded:", n, "keys from", userConfig)
    }
    // write the bundled defaults as a template for the operator
    function saveDefaultConfig()
    {
        var obj = {}
        var n = 0
        for (var key in instrumentConfig) {
            if(key === "objectName" || typeof(instrumentConfig[key]) === "function") continue
            obj[key] = instrumentConfig[key]
            n++
        }
        if(n > 0 && application.saveJson(userConfig, JSON.stringify(obj, null, 2)))
            console.log("EMIP: default config written:", n, "keys to", userConfig)
    }
    Component.onCompleted: loadUserConfig()

    EmipTelemetry {
        id: instrumentData
        unit: apx.fleet.current
        config: instrumentConfig
    }

    sourceComponent: EmipPanel {
        telemetry: instrumentData
        config: instrumentConfig
    }
}
