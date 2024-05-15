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
import QtLocation
import QtPositioning

import Apx.Map
import Apx.Map.Common

MapObject {
    id: control
    implicitZ: -100

    implicitCoordinate: QtPositioning.coordinate(modelData.lat,modelData.lon)
    title: modelData.title


    textColor: "white"
    color: Style.cGreen //"white"
    hoverScaleFactor: 1
    opacity: ui.effects?((hover||selected)?1:0.7):1

    onTriggered: {
        sites.editor.trigger() //({"pos":Qt.point(0,ui.window.height)})
    }

    //dragging support
    onMovingFinished: {
        if(selected){
            var d=modelData
            d.lat=coordinate.latitude
            d.lon=coordinate.longitude
            sites.lookup.model.set(index,d)
        }
    }
    draggable: selected

    onSelectedChanged: {
        if(selected){
            sites.createEditor(modelData)
        }else{
            sites.destroyEditor(modelData)
        }
    }
}
