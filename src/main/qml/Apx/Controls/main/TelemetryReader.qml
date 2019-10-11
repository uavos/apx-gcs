import QtQuick 2.12

import Apx.Common 1.0

FactButton {
    fact: apx.vehicles.replay.telemetry.reader
    expandable: false
    noFactTrigger: true
    onTriggered: {
        apx.vehicles.replay.telemetry.trigger()
        //apx.vehicles.replay.telemetry.reader.loadCurrent()
    }
}
