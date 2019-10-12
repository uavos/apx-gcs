import QtQuick 2.12

import Apx.Common 1.0

FactButton {
    fact: apx.vehicles.replay.telemetry.reader
    status: ""
    titleSize: 0.45
    expandable: false
    noFactTrigger: true
    onTriggered: {
        apx.vehicles.replay.telemetry.trigger()
    }
}
