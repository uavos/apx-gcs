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

import APX.Fleet as APX

MapPolyline {
    opacity: ui.effects?0.8:1
    line.width: replay?1.5:1.5
    line.color: replay
                ? Style.cBlue
                : unit.telemetry.active
                  ? Style.cLineGreen
                  : Style.cBlue

    property APX.Unit unit: apx.fleet.current

    property bool replay: unit.isReplay

    onUnitChanged: updatePath()

    Connections {
        target: unit
        function onGeoPathAppend(p){ addCoordinate(p) }
    }
    Connections {
        enabled: replay
        target: unit
        function onGeoPathChanged(){ updatePath() }
    }
    Connections {
        target: unit.telemetry.rpath
        function onTriggered(){ setPath(QtPositioning.path()) }
    }

    function updatePath()
    {
        setPath(unit.geoPath)
    }

    function showRegion()
    {
        if(path.length>0){
            map.showRegion(unit.geoPathRect())
        }
    }


    Connections {
        enabled: replay
        target: apx.fleet.replay.telemetry.reader
        function onTriggered(){ showRegion() }
    }
    Component.onCompleted: updatePath()
}
