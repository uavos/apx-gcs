import QtQuick 2.12

import Apx.Application 1.0
import Apx.Map.Controls 1.0

AppPlugin {
    id: plugin
    sourceComponent: MissionPlanner { }

    state: "maximized"

    uiComponent: "main"
    onConfigure: {
        ui.main.add(plugin, GroundControl.Layout.Main, 100)
    }
}
