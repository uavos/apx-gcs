import QtQuick

import Apx.Application

/*

    Simplified main layout.

    Install this file as `~/Documents/UAVOS/Plugins/main/GroundControl.qml`,
    the application loads it instead of the built-in layout at startup.

    Plugins listed below are switched off every time the application starts
    and their tools, windows and settings are hidden right away.

*/

GroundControl {

    // nodes button, controls group and choke in the commands panel
    hiddenItems: ["nodes", "controls", "choke"]

    // plugins switched off at startup
    readonly property var disabledPlugins: [
        "signals",          // realtime chart
        "videostreaming",   // camera
        "ekf",
        "parameters",
        "mandalatree",
        "scriptcompiler",
        "datalinkinspector",
        "vcpforwarding",
        "appupdate",
    ]

    // tools removed from the Tools menu
    readonly property var hiddenTools: [
        "terminal",
        "script",
        "datalinkinspector",
        "vcpforwarding",
        "videostreaming",
        "ekf",
        "parameters",
        "mandalatree",
    ]

    // menu entries and settings hidden from the user
    readonly property var hiddenFacts: [
        apx.sysmenu.child("windows"),
        apx.windows,
        apx.settings.application.child("plugins"),
        apx.settings.application.child("appupdate"),
        apx.sysmenu.child("app").child("update"),
    ]

    // unit menu entries hidden from the user, for every unit incl. discovered later
    readonly property var hiddenUnitFacts: ["mandala", "nodes"]

    function hideUnitFacts(unit) {
        for (var i = 0; i < hiddenUnitFacts.length; ++i) {
            var f = unit.child(hiddenUnitFacts[i])
            if (f)
                f.visible = false
        }
    }

    Connections {
        target: apx.fleet
        function onUnitRegistered(unit) { hideUnitFacts(unit) }
    }

    Component.onCompleted: {
        var plugins = apx.settings.application.child("plugins")
        for (var i = 0; i < disabledPlugins.length; ++i) {
            var name = disabledPlugins[i]
            var p = plugins.child(name)
            if (p && p.value)
                p.value = false
            // window launcher of a plugin loaded before this file
            var w = apx.windows.child(name)
            if (w) {
                // a window restored on startup must be closed first:
                // a detached launcher crashes AppWindow::saveStateDo on move/resize
                w.value = false
                if (!w.value)
                    w.parentFact = null
            }
        }
        for (var j = 0; j < hiddenTools.length; ++j) {
            var t = apx.tools.child(hiddenTools[j])
            if (t)
                t.parentFact = null
        }
        for (var k = 0; k < hiddenFacts.length; ++k) {
            if (hiddenFacts[k])
                hiddenFacts[k].visible = false
        }
        for (var u = 0; u < apx.fleet.size; ++u)
            hideUnitFacts(apx.fleet.child(u))    }
}
