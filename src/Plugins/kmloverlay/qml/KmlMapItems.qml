import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtQml 2.12

MapItemGroup {
    id: places

    property var map: ui.map

    MapItemView {
        id: borderPointsView
        model: apx.tools.kmloverlay.kmlPolygons

        z: 1
        delegate: MapPolygon {
            path: polygon
            color: "red"

//            onPathChanged: console.log(path.length)
//            Component.onCompleted: console.log(path)
        }
    }

    Component.onCompleted: {
//        apx.tools.sites.lookup.area=Qt.binding(function(){return apx.tools.map.area})
        map.addMapItemView(borderPointsView)
    }
}
