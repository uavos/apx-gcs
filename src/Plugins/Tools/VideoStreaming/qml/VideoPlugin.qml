import QtQuick 2.12

import Apx.Common 1.0
import Apx.Application 1.0

AppPlugin {
    id: pluginLoader
    sourceComponent: Video { }
    uiComponent: "main"
    onConfigure: {
        ui.main.add(pluginLoader, GroundControl.Layout.MainWidget, 1)
        ui.main.add(pluginLoader, GroundControl.Layout.Main, 1)
    }
}
