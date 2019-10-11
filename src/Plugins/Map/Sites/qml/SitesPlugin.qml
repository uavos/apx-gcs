import QtQuick 2.5

import Apx.Application 1.0
import Apx.Map 1.0

AppPlugin {
    id: pluginSites
    sourceComponent: Component { SitesMapItems { } }

    uiComponent: "map"
    onConfigure: parent=ui.map
    onLoaded: ui.map.addMapItemGroup(item)
}
