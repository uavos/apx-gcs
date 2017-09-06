import QtQuick 2.2
import "."


Item {
    id: toolbar
    property int sz: 32*mapProvider.mapScaleFactor*mapProvider.itemScaleFactor
    width: Math.min(sz,height/column.children.length-column.spacing)
    Column {
        id: column
        width: parent.width
        spacing: 10*mapProvider.mapScaleFactor
        /*MapButton {
            icon: "ios-list"
            onClicked: mission.clear();
        }*/
        MapButton {
            icon: "android-menu"
            //onClicked: mission.request();
        }
        MapButton {
            icon: "android-locate"
            //onClicked: mission.upload();
        }
        MapButton {
            icon: "ios-plus"
            onClicked: map.zoomIn(mapProvider.center);
        }
        MapButton {
            icon: "ios-minus"
            onClicked: map.zoomOut(mapProvider.center);
        }
        /*MapButton {
            icon: "more"
            //onClicked: mission.request();
        }*/
    }

}
