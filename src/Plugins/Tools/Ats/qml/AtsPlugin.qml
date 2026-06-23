import QtQuick

import Apx.Application

AppPlugin {
    id: plugin

    sourceComponent: Component {
        AtsBeam { }
    }

    uiComponent: "map"
    onConfigure: parent = ui.map
    onLoaded: if (ui.map) ui.map.addMapItemGroup(item)
}
