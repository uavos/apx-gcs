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

import APX.Fleet as APX
import Apx.Common

ValueButton {
    id: control

    text: qsTr("LOS")
    toolTip: qsTr("Line of Sight distance to Home")

    readonly property real m_dist: QtPositioning.coordinate(mandala.fact("est.ref.lat").value,mandala.fact("est.ref.lon").value).distanceTo(apx.fleet.current.coordinate)
    readonly property real m_hmsl: mandala.fact("est.pos.hmsl").value
    readonly property real m_ref_hmsl: mandala.fact("est.ref.hmsl").value
    readonly property real m_rss: mandala.fact("sns.com.rss").value

    property double v: Math.sqrt(Math.pow(m_dist,2) + Math.pow(m_hmsl-m_ref_hmsl,2))
    value: v>1000000?"--":apx.distanceToString(v)


    readonly property APX.Unit unit: apx.fleet.current
    readonly property int err: unit.protocol?unit.protocol.errcnt:0
    
    valueScale: 0.8

    Timer {
        id: errTimer
        interval: 5000
        repeat: false
    }

    Connections {
        target: apx.fleet
        function onUnitSelected() {
            cv = unit
        }
    }
    property var cv
    Component.onCompleted: {
        cv = unit
        visible=bvisible
    }

    onErrChanged: {
        if(err<=0) return
        if(cv !== unit){
            cv = unit
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
