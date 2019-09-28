import QtQuick 2.5
import QtQml 2.12
import QtLocation 5.9
import QtPositioning 5.6


import "../lib"
import ".."

MapObject {  //to be used inside MapComponent only
    id: missionObject

    property var fact: null

    visible: mission.visible //&& visibleOnMap

    draggable: selected

    Connections {
        target: fact?fact:null
        enabled: fact!==null
        onTriggered: selectAndCenter()
    }

    onMenuRequested: {
        if(fact) fact.trigger() //({"pos":Qt.point(ui.window.width/4,ui.window.height/2)})
    }

    property int detailsLevel: 20
    Connections {
        target: map
        onZoomLevelChanged: dlTimer.start()
    }
    Timer {
        id: dlTimer
        interval: 1000
        onTriggered: detailsLevel=map.zoomLevel
    }

    //Fact bindings
    title: fact?fact.num+1:0
    implicitCoordinate: fact?fact.coordinate:QtPositioning.coordinate()

    property real f_altitude: (fact && fact.altitude)?fact.altitude.value:0
    property bool active: fact?fact.active:false

    //dragging support
    onMoved: {
        if(fact){
            fact.coordinate=coordinate
        }
    }
}
