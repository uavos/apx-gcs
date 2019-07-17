import QtQuick 2.5
import QtLocation 5.13
import QtPositioning 5.13
import QtQml 2.12

MapItemGroup {
    id: places

    property var map: ui.map
    property var area: apx.tools.map.area
    property var kmlCenter: apx.tools.kmloverlay.center
    onKmlCenterChanged: {
        ui.map.center = kmlCenter;
    }
    onAreaChanged: apx.tools.kmloverlay.updateKmlModels(area)

    MapItemView {
        id: borderPointsView
        model: apx.tools.kmloverlay.kmlPolygons

        z: 1
        delegate: MapPolygon {
            color: polygonColor
            geoShape: polygon
        }
    }

    Component.onCompleted: {
        map.addMapItemView(borderPointsView)

    }
}
