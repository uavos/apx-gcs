import QtQuick 2.5;

import Apx.Common 1.0

FactValue {

    property bool light: false

    alerts: true

    onDefaultWidthChanged: timerWidthUpdate.start()

    property Timer timerWidthUpdate: Timer {
        interval: 1
        onTriggered: {
            implicitWidth=Math.max(implicitWidth,defaultWidth)
        }
    }

    //valueScale: light?0.7:1
    normalColor: light?"#555":normalColor
    //implicitHeight: 0
}
