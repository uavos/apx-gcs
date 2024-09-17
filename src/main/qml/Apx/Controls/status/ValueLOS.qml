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
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtPositioning

import APX.Vehicles as APX
import Apx.Common

ValueButton {
    id: control

    text: qsTr("LOS")
    toolTip: qsTr("Line of Sight distance to Home")

    readonly property real m_dist: QtPositioning.coordinate(mandala.est.ref.lat.value,mandala.est.ref.lon.value).distanceTo(apx.vehicles.current.coordinate)
    readonly property real m_hmsl: mandala.est.pos.hmsl.value
    readonly property real m_ref_hmsl: mandala.est.ref.hmsl.value
    readonly property real m_rss: mandala.sns.com.rss.value

    property double v: Math.sqrt(Math.pow(m_dist,2) + Math.pow(m_hmsl-m_ref_hmsl,2))
    value: v>1000000?"--":apx.distanceToString(v)


    readonly property APX.Vehicle vehicle: apx.vehicles.current
    readonly property int err: vehicle.protocol?vehicle.protocol.errcnt:0
    
    valueScale: 0.8

    Timer {
        id: errTimer
        interval: 5000
        repeat: false
    }

    Connections {
        target: apx.vehicles
        function onVehicleSelected() {
            cv = vehicle
        }
    }
    property var cv
    Component.onCompleted: {
        cv = vehicle
        visible=bvisible
    }

    onErrChanged: {
        if(err<=0) return
        if(cv !== vehicle){
            cv = vehicle
            return
        }
        errTimer.restart()
    }

    error: errTimer.running && err>0
    warning: m_rss>0 && (m_rss<0.35 || v>70000)
    alerts: true


    property bool bvisible: ui.test || v>0 || error || warning
    visible: false
    onBvisibleChanged: if(bvisible)visible=true

}
