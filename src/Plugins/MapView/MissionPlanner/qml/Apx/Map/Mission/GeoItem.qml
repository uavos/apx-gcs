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
import QtQuick.Effects
import QtLocation
import QtPositioning

import Apx.Map.Common

MissionObject {
    id: geoItem

    color: fact?fact.opts.color:"#FF8000"
    textColor: "white"
    fact: modelData
    implicitZ: 30
    opacity: (hover || selected)?1:ui.effects?0.5:1
    title: fact?fact.title:"GEO"

    //Fact bindings
    property int f_radius: Math.abs(fact?fact.radius.value:0)


    //Map Items
    property bool circleActive: selected||dragging||hover

    Loader {
        // asynchronous: true
        onLoaded: map.addMapItem(item)
        sourceComponent: Component {
            MapCircle {
                //z: -100
                color: circleActive?"#2040FF40":"#100000FF"
                border.color: "#500000FF"
                border.width: 1
                radius: geoItem.f_radius
                center: geoItem.coordinate
                Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
            }
        }
    }
}
