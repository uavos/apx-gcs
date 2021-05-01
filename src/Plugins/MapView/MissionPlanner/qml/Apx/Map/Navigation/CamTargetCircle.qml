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
import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0
import Apx.Map.Common 1.0

MapItemGroup {
    visible: valid

    //Fact bindings
    property real lat: mandala.est.pos.lat.value
    property real lon: mandala.est.pos.lon.value
    property real hmsl: mandala.est.pos.hmsl.value
    property real home_hmsl: mandala.est.ref.hmsl.value

    property real yaw: mandala.est.att.yaw.value
    property bool power_payload: mandala.ctr.pwr.payload.value === pwr_payload_on

    property real cam_roll: mandala.est.cam.roll.value
    property real cam_pitch: mandala.est.cam.pitch.value
    property real cam_yaw: mandala.est.cam.yaw.value

    //calculate
    property bool valid: power_payload
                         && camAlt>0
                         && (cam_roll!==0 || cam_pitch!==0 || cam_yaw!==0)

    property bool bCamFront: cam_roll !== 0

    property real camAlt: hmsl-home_hmsl

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
            text: apx.distanceToString(targetCoordinate.distanceTo(uavCoordinate))
            color: circle.color
        }
    }

}
