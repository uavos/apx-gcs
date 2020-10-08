import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import QtPositioning 5.12

import APX.Vehicles 1.0
import Apx.Common 1.0

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


    property int err: apx.vehicles.current.protocol.errcnt
    Timer {
        id: errTimer
        interval: 5000
        repeat: false
    }

    Connections {
        target: apx.vehicles
        function onVehicleSelected() {
            cv = apx.vehicles.current
        }
    }
    property var cv
    Component.onCompleted: {
        cv = apx.vehicles.current
        visible=bvisible
    }

    onErrChanged: {
        if(apx.vehicles.current.errcnt<=0) return
        if(cv !== apx.vehicles.current){
            cv = apx.vehicles.current
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
