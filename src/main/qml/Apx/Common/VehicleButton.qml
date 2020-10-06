import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
//import QtLocation 5.9
import QtPositioning 5.6

import APX.Vehicles 1.0

import "Button"

ButtonBase {
    id: control
    property var vehicle

    size: 48 * ui_scale
    font.pixelSize: Math.max(7, height * 0.33 - 2)

    highlighted: vehicle.active
    visible: vehicle.visible

    color: _label.colorBG

    contentItem: VehicleLabel {
        id: _label
        font: control.font
        vehicle: control.vehicle
    }


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
