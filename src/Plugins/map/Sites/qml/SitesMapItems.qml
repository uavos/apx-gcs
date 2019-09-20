import QtQuick 2.5
import QtLocation 5.9
import QtPositioning 5.6
import QtQml 2.12


MapItemGroup {
    id: places
    property bool showPlaces: apx.tools && sites
    property var map: ui.map
    property var sites: apx.tools.Sites

    z: 1

    MapItemView {
        id: sitesView
        model: places.showPlaces?sites.lookup.dbModel:0
        delegate: SiteItem { }
    }

    Component.onCompleted: {
        sites.lookup.area=Qt.binding(function(){return map.area})
        map.addMapItemView(sitesView)
    }


    //triggered site in lookup - focus on map
    Connections {
        target: sites.lookup
        onItemTriggered: map.showCoordinate(QtPositioning.coordinate(modelData.lat,modelData.lon))
    }


    //CURRENT SITE LABEL
    Text {
        //z: -1000
        parent: map //ui.main
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: ui.main.containerBottom.height?ui.main.containerBottom.height+20:0
        visible: text
        font.pixelSize: 24
        font.bold: true
        color: "#fff"
        text: apx.vehicles.current.mission.site
    }

}
