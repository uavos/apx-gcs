import QtQuick 2.12

import Apx.Common 1.0
import Apx.Application 1.0

AppPlugin {
    id: pluginLoader
    sourceComponent: Video { }
    uiComponent: "main"
    state: "minimized"
    onConfigure: {
        ui.main.add(pluginLoader, GroundControl.Layout.Main, 100)
    }
}
