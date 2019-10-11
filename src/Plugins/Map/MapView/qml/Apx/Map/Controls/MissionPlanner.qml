import QtQuick 2.12

import Apx.Application 1.0
import Apx.Map.MapView 1.0

MapView {
    id: control

    //initial animation
    PropertyAnimation {
        running: true
        target: map
        property: "zoomLevel"
        from: 12
        to: 16.33
        duration: 1000
        easing.type: Easing.OutInCirc
    }

    Component.onCompleted: {
        application.registerUiComponent(map,"map")
        ui.main.add(control, GroundControl.Layout.Background)
    }
    onMapBackgroundItemLoaded: {
        application.registerUiComponent(item,"mapbase")
    }


    //Controls
    /*Component.onCompleted: {
        ui.main.add(wind, GroundControl.Layout.Info)
        ui.main.add(info, GroundControl.Layout.Status)
    }
    Loader {
        id: wind
        active: showWind && m.windSpd.value>0
        asynchronous: true
        sourceComponent: Component { Wind { } }
        visible: status===Loader.Ready
    }
    Loader {
        id: info
        active: showInfo
        asynchronous: true
        sourceComponent: Component { MapInfo { } }
        //onLoaded: ui.main.mainLayout.addInfo(item)
    }*/

}
