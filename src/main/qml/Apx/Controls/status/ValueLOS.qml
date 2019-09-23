import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2

import APX.Vehicles 1.0
import Apx.Common 1.0

FactValue {
    id: control
    title: qsTr("LOS")

    descr: qsTr("Line of Sight distance to Home")
    property double v: Math.sqrt(Math.pow(m.dHome.value,2) + Math.pow(m.gps_hmsl.value-m.home_hmsl.value,2))
    value: v>=1000?(v/1000).toFixed(1):v.toFixed()

    property int err: apx.vehicles.current.mandala.errcnt
    Timer {
        id: errTimer
        interval: 5000
        repeat: false
    }

    Connections {
        target: apx.vehicles
        onVehicleSelected: {
            cv = apx.vehicles.current
        }
    }
    property var cv
    Component.onCompleted: {
        cv = apx.vehicles.current
        visible=bvisible
    }

    onErrChanged: {
        if(m.errcnt<=0) return
        if(cv != apx.vehicles.current){
            cv = apx.vehicles.current
            return
        }
        errTimer.restart()
    }

    error: errTimer.running && err>0
    warning: m.RSS>0 && (m.RSS<0.35 || v>70000)
    alerts: true


    property bool bvisible: ui.test || v>0 || error || warning
    visible: false
    onBvisibleChanged: if(bvisible)visible=true

}
