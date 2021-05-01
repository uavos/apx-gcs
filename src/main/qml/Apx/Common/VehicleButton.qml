/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtPositioning 5.6

import APX.Vehicles 1.0 as APX

import "Button"

ButtonBase {
    id: control
    
    property APX.Vehicle vehicle

    size: Style.buttonSize * 1.6

    highlighted: vehicle.active
    visible: vehicle.visible

    color: _label.colorBG

    contentItem: VehicleLabel {
        id: _label
        fontSize: control.size*0.31
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
