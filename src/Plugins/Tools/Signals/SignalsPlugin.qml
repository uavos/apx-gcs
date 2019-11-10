import QtQuick 2.12

import Apx.Common 1.0
import Apx.Application 1.0

AppPlugin {
    id: plugin

    title: qsTr("Signals")
    descr: qsTr("Realtime chart")
    icon: "poll"

    sourceComponent: Signals { }
    uiComponent: "main"
    onConfigure: {
        ui.main.add(plugin, GroundControl.Layout.Main)
    }
}
