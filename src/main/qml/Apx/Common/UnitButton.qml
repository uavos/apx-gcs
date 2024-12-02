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
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtPositioning

import APX.Fleet as APX

import "Button"

ButtonBase {
    id: control
    
    property APX.Unit unit

    size: Style.buttonSize * 1.6

    highlighted: unit.active
    visible: unit.visible

    color: _label.colorBG

    contentItem: UnitLabel {
        id: _label
        fontSize: control.size*0.31
        unit: control.unit
    }


    onClicked: {
        if(unit.active){
            unit.trigger()
        }else{
            apx.fleet.selectUnit(unit);
        }
    }

    onDoubleClicked: {
        if(unit.active){
            unit.follow=true
        }
    }
    onPressAndHold: {
        if(unit.active){
            unit.follow=true
        }
    }
}
