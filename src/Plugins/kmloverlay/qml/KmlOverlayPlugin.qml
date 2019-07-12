import QtQuick 2.5

import Apx.Common 1.0

AppPlugin {
    id: pluginKmlOverlay
    sourceComponent: Component { KmlMapItems { } }

    uiComponent: "map"
    onConfigure: parent = ui.map
    onLoaded: ui.map.addMapItemGroup(item)
}
