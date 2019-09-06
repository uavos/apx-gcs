import QtQuick 2.13
import QtLocation 5.13
import QtPositioning 5.13
import QtQml 2.13
import KmlGeoPolygon 1.0

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
        delegate: MapQuickItem
        {
            coordinate: apx.tools.map.area.boundingGeoRectangle().topLeft

            sourceItem: KmlGeoPolygon {
                id: kmlPolygon
                area: apx.tools.map.area
                map: ui.map
                geoPolygon: polygon
                color: polygonColor
                opacity: apx.tools.kmloverlay.opacity.value
            }
        }
    }

    Component.onCompleted: {
        map.addMapItemView(borderPointsView)
    }
}
