import QtQuick 2.12

import Apx.Common 1.0
import Apx.Instruments 1.0
import Apx.Application 1.0

AppPlugin {
    id: plugin

    title: qsTr("Heading")
    descr: qsTr("Navigation instrument")
    icon: "navigation"

    sourceComponent: Hdg { }
    uiComponent: "main"
    onConfigure: {
        ui.main.add(plugin, GroundControl.Layout.Instrument)
        //ui.main.add(plugin, GroundControl.Layout.Main)
    }
    state: "minimized"
}
