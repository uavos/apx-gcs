import QtQuick 2.5

import Apx.Application 1.0

AppPlugin {
    id: plugin
    sourceComponent: Component { KmlMapItems { } }

    uiComponent: "map"
    onLoaded: updateMap() // when mapbase loads first

    function updateMap()
    {
        if(ui.map) {
            ui.map.addMapItemGroup(plugin.item)
        }
    }

    Connections {
        target: application
        onUiComponentLoaded: updateMap() // when mapbase loads after plugin
    }
}
