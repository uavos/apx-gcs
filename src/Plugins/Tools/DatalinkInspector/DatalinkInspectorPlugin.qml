import QtQuick 2.12

import Apx.Common 1.0
import Apx.Application 1.0

AppPlugin {
    id: plugin

    title: qsTr("Datalink Inspector")
    descr: qsTr("Realtime protocols view")
    icon: "teamviewer"



    sourceComponent: DatalinkInspector { }
    uiComponent: "main"
    onConfigure: {
        ui.main.add(plugin, GroundControl.Layout.Instrument)
    }
}
