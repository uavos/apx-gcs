import QtQuick          2.3
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Window   2.2

import QtQml            2.12
import QtGraphicalEffects 1.0

import APX.Vehicles     1.0

import "../vehicle"
import "../mission"

MapBase {
    id: map

    readonly property real fontSize: Qt.application.font.pixelSize*ui.scale

    //map item setup
    color: 'transparent' // Necessary to make this map transparent
    //plugin: Plugin { name: "itemsoverlay" }
    //gesture.enabled: false
    //focus: false

    //---------------------------
    //helpers
    /*function metersToPixels(meters)
    {
        var coord = toCoordinate(Qt.point(width,height/2))
        var dist = center.distanceTo(coord)
        return meters*(width/2)/dist;
    }*/
    Connections {
        target: map
        onCenterChanged: mpTimer.start()
        onZoomLevelChanged: mpTimer.start()
    }

    property real metersToPixelsFactor: 1
    Timer {
        id: mpTimer
        interval: 500
        onTriggered: {
            var coord = toCoordinate(Qt.point(width,height/2))
            var dist = center.distanceTo(coord)
            metersToPixelsFactor=(width/2)/dist;
        }
    }


    //internal
    property double itemsScaleFactor: 1

    property var selectedObject

    //---------------------------
    //MapTools sync
    onMouseClickCoordinateChanged: {
        if(apx.tools.map) apx.tools.map.clickCoordinate=map.mouseClickCoordinate
    }
    onMenuRequested: {
        if(apx.tools.map){
            apx.tools.map.requestMenu({"pos": mouseClickPoint, "closeOnActionTrigger":true})
        }
    }
    onCenterChanged: mapToolsUpdateTimer.start()
    onZoomLevelChanged: mapToolsUpdateTimer.start()
    Timer {
        id: mapToolsUpdateTimer
        interval: 500
        onTriggered: updateMapToolsArea()
    }
    function updateMapToolsArea()
    {
        apx.tools.map.area=map.visibleRegion
    }

}
