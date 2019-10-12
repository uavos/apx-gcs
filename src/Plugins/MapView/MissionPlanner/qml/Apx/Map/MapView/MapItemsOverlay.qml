import QtQuick          2.12
import QtLocation       5.12
import QtPositioning    5.12


MapBase {
    id: map

    readonly property real fontSize: Qt.application.font.pixelSize*ui.scale

    property var mapPlugin: apx.tools.missionplanner

    //map item setup
    color: 'transparent' // Necessary to make this map transparent

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

    //select tool to trigger on click
    property var selectedTool
    onClicked: {
        if(!selectedTool)return
        selectedTool.trigger()
    }


    //internal
    property real itemsScaleFactor: 1

    property var selectedObject

    //---------------------------
    //MapTools sync
    onMouseClickCoordinateChanged: {
        if(mapPlugin) mapPlugin.clickCoordinate=map.mouseClickCoordinate
    }
    onMenuRequested: {
        if(selectedTool){
            selectedTool=null
            return
        }
        if(!mapPlugin)return
        mapPlugin.trigger({"posXY": mouseClickPoint})
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
        mapPlugin.area=map.visibleRegion
        area=map.visibleRegion
    }
    property var area: QtPositioning.shape()

}
