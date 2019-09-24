import QtQuick 2.13
import QtLocation 5.13
import QtPositioning 5.13
import QtQml 2.13
import KmlGeoPolygon 1.0

MapItemGroup {
    id: control

    property var map: ui.map
    property var area: apx.tools.mapview.area
    property var plugin: apx.tools.kmloverlay
    property var kmlCenter: plugin.center
    onKmlCenterChanged: {
        ui.map.centerOn(kmlCenter)
    }
    onAreaChanged: plugin.updateKmlModels(area)

    MapItemView {
        id: borderPointsView
        model: plugin.kmlPolygons

        z: 1
        delegate: MapQuickItem
        {
            coordinate: control.area.boundingGeoRectangle().topLeft

            sourceItem: KmlGeoPolygon {
                id: kmlPolygon
                area: control.area
                map: ui.map
                geoPolygon: polygon
                color: polygonColor
                opacity: plugin.opacity.value
            }
        }
    }

    Component.onCompleted: {
        map.addMapItemView(borderPointsView)
    }
}
