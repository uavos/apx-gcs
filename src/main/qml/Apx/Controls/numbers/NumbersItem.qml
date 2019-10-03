import QtQuick 2.5;

import Apx.Common 1.0

FactValue {
    id: control
    property bool light: false

    alerts: true
    normalColor: light?"#555":normalColor

    //ensure width only grows
    Component.onCompleted: {
        implicitWidth=0
    }
    onDefaultWidthChanged: timerWidthUpdate.start()
    property Timer timerWidthUpdate: Timer {
        interval: 0
        onTriggered: {
            if(implicitWidth<defaultWidth)
                implicitWidth=defaultWidth

            if(model && model.minimumWidth<defaultWidth)
                model.minimumWidth=defaultWidth
        }
    }

    //update model minimum width
    property var model
    onModelChanged: timerWidthUpdate.start()
}
