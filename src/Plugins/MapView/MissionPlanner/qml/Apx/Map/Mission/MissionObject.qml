import QtQuick 2.12
import QtLocation 5.13
import QtPositioning 5.13

import Apx.Map.Common 1.0
import Apx.Common 1.0

MapObject {
    id: missionObject

    property var fact: null

    visible: mission.visible //&& visibleOnMap

    draggable: selected

    Connections {
        target: fact?fact:null
        enabled: fact!==null
        onTriggered: selectAndCenter()
    }

    onTriggered: {
        if(fact) fact.trigger()
    }

    property int detailsLevel: 20
    Connections {
        target: map
        onZoomLevelChanged: dlTimer.start()
        onSelectedObjectChanged: {
            if(!fact)return
            fact.selected = selected
        }
    }
    Timer {
        id: dlTimer
        interval: 1000
        onTriggered: detailsLevel=map.zoomLevel
    }

    Connections {
        target: mission
        onSelectedItemChanged: {
            if(!fact)return
            if(mission.selectedItem == fact && !selected) selectAndCenter()
        }
    }

    contentsCenter: Loader {
        z: -1
        active: fact?fact.active:false
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: missionObject.width
        sourceComponent: Component {
            MaterialIcon {
                size: missionObject.width*1.8
                name: "chevron-left"
                color: missionObject.color
            }
        }
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
