import QtQuick 2.5;
import QtQuick.Layouts 1.3
import QtLocation 5.9
import QtPositioning 5.6

import APX.Vehicles 1.0
import Apx.Common 1.0

Item {
    id: control
    implicitHeight: vehiclesList.height
    implicitWidth: vehiclesList.width
    Layout.minimumWidth: height
    ListView {
        id: vehiclesList
        model: apx.vehicles.list.model
        implicitHeight: contentItem.childrenRect.height
        implicitWidth: Math.min(contentItem.childrenRect.width,parent.width)
        orientation: ListView.Horizontal
        delegate: VehicleButton {
            enabled: true
            vehicle: modelData
        }

        spacing: 10*ui.scale
        clip: true
        snapMode: ListView.SnapToItem

        header: RowLayout {
            //width: hdr.width+10
            //height: hdr.height
            spacing: vehiclesList.spacing
            VehicleButton {
                enabled: true
                vehicle: apx.vehicles.LOCAL
                menuFact: apx

            }
            VehicleButton {
                enabled: true
                vehicle: apx.vehicles.REPLAY
                menuFact: vehicle

            }
            Item { Layout.fillHeight: true  }
        }

        Component {
            id: vehicleInfoDelegate
            VehicleButton {
                enabled: true
                vehicle: modelData
            }
        }
    }
    //Rectangle { anchors.fill: control; color: "#80ffffff" }
}
