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

import Apx.Map.Common

MapIcon {

    name: "fullscreen-exit"
    color: Style.cYellow

    //Fact bindings
    readonly property real lat: mandala.fact("est.pos.lat").value
    readonly property real lon: mandala.fact("est.pos.lon").value
    readonly property real hmsl: mandala.fact("est.pos.hmsl").value

    readonly property real dn: mandala.fact("est.ins.dn").value
    readonly property real de: mandala.fact("est.ins.de").value
    readonly property real dh: mandala.fact("est.ins.dh").value

    readonly property bool ins_nogps: mandala.fact("cmd.ins.nogps").value


    // calculated properties
    property real azimuth: Math.atan2(-de,-dn)*180/Math.PI
    property real distance: Math.sqrt(dn*dn+de*de)

    coordinate: QtPositioning.coordinate(lat,lon,hmsl-dh).atDistanceAndAzimuth(distance,azimuth)

    visible: distance > 10 || ins_nogps
}
