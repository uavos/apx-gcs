import QtQuick 2.5

import Apx.Common 1.0
import "Apx/Map"

AppPlugin {
    id: plugin
    sourceComponent: Component { ApxMap { } }

    uiComponent: "main"
    onConfigure: {
        ui.main.mainLayout.addMainItem(plugin)
    }

    Loader {
        active: plugin.active && plugin.status==Loader.Ready
        sourceComponent: Component { ApxMapTools { } }
    }
}
