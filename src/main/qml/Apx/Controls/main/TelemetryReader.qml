import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Apx.Common 1.0
import Apx.Menu 1.0

RowLayout {
    id: control
    /*CleanButton {
        iconName: apx.vehicles.replay.telemetry.player.icon
        Layout.fillHeight: true
        toolTip: apx.vehicles.replay.telemetry.player.descr
        onTriggered: apx.vehicles.replay.telemetry.trigger()
    }*/

    FactButton {
        fact: apx.vehicles.replay.telemetry.reader
        Layout.fillHeight: true
        expandable: false
    }

}
