import QtQuick 2.5

import Apx.Common 1.0
//import Apx.Map 1.0
import "Apx/Map"

AppPlugin {
    id: plugin
    sourceComponent: ApxMap { showVehicles: true }

    uiComponent: "main"
    onConfigure: {
        parent=ui.main.containerMain
        anchors.fill=parent
    }
    Component.onDestruction: {
        //console.log("map destroyed")
    }
}
