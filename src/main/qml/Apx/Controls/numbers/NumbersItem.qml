import QtQuick 2.5;

import Apx.Common 1.0

FactValue {
    id: control
    property bool light: false

    alerts: true
    normalColor: light?"#555":normalColor

    //ensure width only grows
    Component.onCompleted: {
        implicitWidth=height
        updateWidth()
    }

    function updateWidth()
    {
        if(implicitWidth<defaultWidth){
            implicitWidth=Qt.binding(function(){return defaultWidth})
            //implicitWidth=defaultWidth
        }

        if(model && model.minimumWidth<defaultWidth)
            model.minimumWidth=defaultWidth
    }

    onDefaultWidthChanged: timerWidthUpdate.start()
    property Timer timerWidthUpdate: Timer {
        //running: true
        interval: 0
        onTriggered: updateWidth()
    }

    //update model minimum width
    property var model
    onModelChanged: timerWidthUpdate.start()
}
