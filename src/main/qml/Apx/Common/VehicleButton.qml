import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
//import QtLocation 5.9
import QtPositioning 5.6

import APX.Vehicles 1.0

CleanButton {
    id: control
    property var vehicle
    property var menuFact: vehicle

    implicitHeight: label.implicitHeight+padding*2

    highlighted: vehicle.active
    visible: vehicle.visible

    color: label.colorBG


    contents: [
        VehicleLabel {
            id: label
            font: control.font
            vehicle: control.vehicle
        }
    ]

    onClicked: {
        if(vehicle.active){
            vehicle.trigger()
        }else{
            apx.vehicles.selectVehicle(vehicle);
        }
    }

    onDoubleClicked: {
        if(vehicle.active){
            vehicle.follow=true
        }
    }
    onPressAndHold: {
        if(vehicle.active){
            vehicle.follow=true
        }
    }
}
