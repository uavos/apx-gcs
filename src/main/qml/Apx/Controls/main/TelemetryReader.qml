import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Apx.Common 1.0
import Apx.Menu 1.0

RowLayout {
    id: control
    CleanButton {
        iconName: apx.vehicles.REPLAY.telemetry.player.icon
        //implicitHeight: control.height
        Layout.fillHeight: true
        toolTip: apx.vehicles.REPLAY.telemetry.player.descr
        onTriggered: apx.vehicles.REPLAY.telemetry.requestMenu()
    }

    FactButton {
        fact: apx.vehicles.REPLAY.telemetry.reader
        Layout.fillHeight: true
        expandable: false
        //implicitHeight: 18
        //implicitHeight: control.height
        onTriggered: fact.requestMenu()
    }

}
