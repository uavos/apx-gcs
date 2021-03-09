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
import QtLocation 5.13

import APX.Vehicles 1.0 as APX
import APX.Mission 1.0 as APX


MapItemGroup {
    id: group
    z: 100

    Component.onCompleted: {
        map.addMapItemGroup(group)
    }

    property APX.Vehicle vehicle: apx.vehicles.current
    property APX.Mission mission: vehicle.mission

    visible: !mission.empty

    Connections {
        enabled: !map.follow
        target: mission
        function onMissionAvailable(){ showRegion() }
    }

    function showRegion()
    {
        //if(mission.empty) return
        var r=mission.boundingGeoRectangle().united(vehicle.geoPathRect())

        if(!map.visibleRegion.boundingGeoRectangle().intersects(r))
            map.showRegion(r)
    }



    MapItemView {
        z: 50
        model: mission.wp.mapModel
        delegate: WaypointItem { }
    }

    MapItemView {
        z: 60
        model: mission.rw.mapModel
        delegate: RunwayItem { }
    }

    MapItemView {
        z: 0
        model: mission.tw.mapModel
        delegate: TaxiwayItem { }
    }

    MapItemView {
        z: 20
        model: mission.pi.mapModel
        delegate: PointItem { }
    }
}
