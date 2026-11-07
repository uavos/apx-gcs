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

    showWindowsButton: false
    showFleetControls: false
    showControlsMenu: false

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
        apx.sysmenu.child("fleet"),
        apx.windows,
        apx.fleet,
        apx.settings.application.child("plugins"),
        apx.settings.application.child("appupdate"),
        apx.sysmenu.child("app").child("update"),
    ]

    Component.onCompleted: {
        var plugins = apx.settings.application.child("plugins")
        for (var i = 0; i < disabledPlugins.length; ++i) {
            var name = disabledPlugins[i]
            var p = plugins.child(name)
            if (p && p.value)
                p.value = false
            // window launcher of a plugin loaded before this file
            var w = apx.windows.child(name)
            if (w)
                w.parentFact = null
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
    }
}
