import QtQuick 2.5;
import QtQuick.Controls.Material 2.12

import Apx.Common 1.0

ValueButton {
    id: control
    property bool light: false

    alerts: true
    normalColor: light?"#555":normalColor

    //highlighted: true


    property string title: fact?fact.name:""
    text: title

    //ensure width only grows
    Component.onCompleted: {
        implicitWidth=height
        updateWidth()

        //_con.enabled=true
    }

    /*Connections {
        id: _con
        target: control
        //enabled: false
        function onDefaultWidthChanged(){ updateWidth() }
        function onModelChanged(){ updateWidth() }
    }*/

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
        interval: 100
        onTriggered: {
            updateWidth()
            interval=0
        }
    }

    //update model minimum width
    property var model
    onModelChanged: timerWidthUpdate.start()
}
