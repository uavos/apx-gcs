import QtQuick 2.5

import Apx.Common 1.0
import "Apx/Map"

AppPlugin {
    id: plugin
    sourceComponent: Component { ApxMap { } }

    uiComponent: "main"
    onConfigure: {
        ui.main.addMainItem(plugin)
        //parent=ui.main.containerMain
        //anchors.fill=parent
    }
    Component.onDestruction: {
        //console.log("map destroyed")
    }
}
