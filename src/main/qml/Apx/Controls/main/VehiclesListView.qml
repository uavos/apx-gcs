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
    Layout.minimumWidth: height*2
    ListView {
        id: vehiclesList
        model: apx.vehicles.list.model
        implicitHeight: contentItem.childrenRect.height
        implicitWidth: Math.min(contentItem.childrenRect.width,parent.width)
        orientation: ListView.Horizontal
        snapMode: ListView.SnapToItem
        delegate: VehicleButton {
            enabled: true
            vehicle: modelData
            Connections {
                target: vehicle
                onSelected: vehiclesList.positionViewAtIndex(index, ListView.Beginning)
            }
        }

        spacing: 10*ui.scale
        clip: true

        header: RowLayout {
            //width: hdr.width+10
            //height: hdr.height
            spacing: vehiclesList.spacing
            VehicleButton {
                enabled: true
                vehicle: apx.vehicles.local
                menuFact: apx

            }
            VehicleButton {
                enabled: true
                vehicle: apx.vehicles.replay
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
