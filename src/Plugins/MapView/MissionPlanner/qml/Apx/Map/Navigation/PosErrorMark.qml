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
    property real lat: mandala.est.pos.lat.value
    property real lon: mandala.est.pos.lon.value
    property real hmsl: mandala.est.pos.hmsl.value

    property real dn: mandala.est.ins.dn.value
    property real de: mandala.est.ins.de.value
    property real dh: mandala.est.ins.dh.value

    property bool ins_nogps: mandala.cmd.ins.nogps.value


    // calculated properties
    property real azimuth: Math.atan2(-de,-dn)*180/Math.PI
    property real distance: Math.sqrt(dn*dn+de*de)

    coordinate: QtPositioning.coordinate(lat,lon,hmsl-dh).atDistanceAndAzimuth(distance,azimuth)

    visible: distance > 10 || ins_nogps
}
