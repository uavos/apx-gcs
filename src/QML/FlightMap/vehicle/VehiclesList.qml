import QtQuick 2.5;
import QtQuick.Layouts 1.3
import QtLocation 5.9
import QtPositioning 5.6
import GCS.Vehicles 1.0
import "."

Item {
    implicitHeight: vehiclesList.height
    implicitWidth: vehiclesList.width
    ListView {
        id: vehiclesList
        model: app.vehicles.list.model
        implicitHeight: contentItem.childrenRect.height
        implicitWidth: Math.min(contentItem.childrenRect.width,parent.width)
        orientation: ListView.Horizontal
        delegate: vehicleInfoDelegate
        spacing: 10
        clip: true
        snapMode: ListView.SnapToItem

        header: RowLayout {
            //width: hdr.width+10
            //height: hdr.height
            spacing: vehiclesList.spacing
            VehicleInfo {
                //id: hdr
                enabled: true
                font.pixelSize: Qt.application.font.pixelSize * 0.8
                vehicle: app.vehicles.LOCAL
                menuFact: app
            }
            VehicleInfo {
                enabled: true
                font.pixelSize: Qt.application.font.pixelSize * 0.8
                vehicle: app.vehicles.REPLAY
                //menuFact: app
            }
            Item { Layout.fillHeight: true  }
        }

        Component {
            id: vehicleInfoDelegate
            VehicleInfo {
                enabled: true
                font.pixelSize: Qt.application.font.pixelSize * 0.8
            }
        }
    }
}
