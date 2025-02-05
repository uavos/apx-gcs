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

import Apx.Common
import Apx.Map.Common

Loader {
    active: power_payload && camAlt>0 && (cam_roll!==0 || cam_pitch!==0 || cam_yaw!==0)


    readonly property bool power_payload: mandala.fact("ctr.pwr.payload").value > 0

    readonly property real cam_roll: mandala.fact("est.cam.roll").value
    readonly property real cam_pitch: mandala.fact("est.cam.pitch").value
    readonly property real cam_yaw: mandala.fact("est.cam.yaw").value

    readonly property real hmsl: mandala.fact("est.pos.hmsl").value
    readonly property real home_hmsl: mandala.fact("est.ref.hmsl").value
    readonly property real camAlt: hmsl-home_hmsl

    onItemChanged:
        if(item)
            map.addMapItemGroup(item)


    sourceComponent: MapItemGroup {
        // TODO optimize JS calculations

        //Fact bindings
        //calculate
        readonly property real yaw: mandala.fact("est.att.yaw").value

        readonly property real lat: mandala.fact("est.pos.lat").value
        readonly property real lon: mandala.fact("est.pos.lon").value

        readonly property bool bCamFront: cam_roll !== 0


        property real azimuth: bCamFront?yaw:cam_yaw

        property real distance: camAlt*Math.tan((90+cam_pitch)*Math.PI/180)

        property var uavCoordinate: QtPositioning.coordinate(lat,lon,hmsl)
        property var targetCoordinate: QtPositioning.coordinate(lat,lon,home_hmsl).atDistanceAndAzimuth(distance,azimuth)

        MapCircle {
            id: circle
            color: "#fff"
            border.width: 0
            radius: 200
            opacity: 0.3
            center: targetCoordinate

            Behavior on center {
                enabled: ui.smooth
                CoordinateAnimation {
                    duration: 1000
                }
            }
        }

        MapIcon {
            name: "circle-medium"
            coordinate: circle.center
            color: circle.color
        }

        MapQuickItem {
            coordinate: circle.center

            anchorPoint.x: text.implicitWidth/2
            anchorPoint.y: -text.implicitHeight

            sourceItem: Text {
                id: text
                font: apx.font_narrow(Style.fontSize)
                readonly property int d: targetCoordinate.distanceTo(uavCoordinate)/10
                text: apx.distanceToString(d*10)
                color: circle.color
            }
        }

    }
}
