import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import Apx.Common 1.0

Rectangle {

    readonly property int m_mode: mandala.cmd.op.mode.value
    readonly property int m_mtype: mandala.est.ctr.mtype.value
    readonly property real m_adj: mandala.cmd.op.adj.value
    readonly property int m_agl_status: mandala.sns.agl.status.value

    readonly property var f_hmsl: mandala.est.pos.hmsl
    readonly property var f_wpt_dist: mandala.est.wpt.dist
    readonly property var f_eta: mandala.est.wpt.eta
    readonly property var f_energy: mandala.est.sys.energy
    readonly property var f_wpidx: mandala.est.wpt.wpidx
    readonly property var f_loops: mandala.est.ctr.loops
    readonly property var f_tdist: mandala.est.ctr.tdist
    readonly property var f_radius: mandala.est.ctr.radius
    readonly property var f_agl: mandala.est.pos.agl


    readonly property bool m_agl_show: m_agl_status !== agl_status_unknown
                                       && m_agl_status !== agl_status_unavailable

    readonly property bool m_agl_warning: m_agl_status===agl_status_warning
    readonly property bool m_agl_failure: m_agl_status===agl_status_failure
    readonly property bool m_agl_ready: m_agl_status===agl_status_ready

    border.width: 0
    color: "#000"
    implicitWidth: itemHeight*4
    implicitHeight: layout.implicitHeight

    readonly property int margins: 3
    readonly property real itemHeight: height/15//*ui.scale

    property bool isLanding:
        m_mode===op_mode_LANDING ||
        m_mode===op_mode_TAKEOFF ||
        m_mode===op_mode_TAXI ||
        (m_mode===op_mode_WPT && m_mtype===ctr_mtype_line)

    ProgressBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 3
        property int v: apx.progress
        to: 100
        value: v
        visible: v>=0
        indeterminate: v==0
        Material.accent: Material.color(Material.Green)
    }

    ColumnLayout{
        id: layout
        width: parent.width
        ColumnLayout {
            spacing: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 3

            ValueRss {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }

            ValueDL {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }

            Loader {
                asynchronous: true
                active: apx.datalink.server.text
                visible: active
                sourceComponent: Component {
                    FactValue {
                        title: qsTr("RC")
                        fact: apx.datalink.server
                        active: false
                        value: fact.text
                        valueScale: 0.8
                        valueColor: fact.extctr.value?Material.color(Material.LightGreen):Material.color(Material.Red)
                        enabled: true
                    }
                }
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }

            Loader {
                asynchronous: true
                active: apx.datalink.hosts.text
                visible: active
                sourceComponent: Component {
                    FactValue {
                        title: qsTr("RS")
                        fact: apx.datalink.hosts
                        active: false
                        value: fact.text
                        valueScale: 0.8
                        valueColor: apx.datalink.server.extctr.value?Material.color(Material.LightGreen):Material.color(Material.LightRed)
                        enabled: true
                    }
                }
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }

            ValueLOS {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }


            /*FactValue {
            title: qsTr("EXT")
            value: apx.datalink.hosts.availableCount
        }
        FactValue {
            title: qsTr("CTR")
            //value
        }*/

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }

            FactValue {
                title: qsTr("H")
                fact: f_hmsl
                property real v: fact.value*(3.281/100)
                value: "FL"+v.toFixed()
                visible: ui.test || v>0
                valueScale: 0.8
                valueColor: Material.color(Material.BlueGrey)
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("DME")
                fact: f_wpt_dist
                property real v: fact.value
                value: v>=1000?(v/1000).toFixed(1):v.toFixed()
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("ETA")
                fact: f_eta
                property int v: fact.value
                property real valid: v>0
                property int tsec: ("0"+Math.floor(v%60)).slice(-2)
                property int tmin: ("0"+Math.floor(v/60)%60).slice(-2)
                property int thrs: Math.floor(v/60/60)
                property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
                value: valid?sETA:"--:--"
                valueScale: 0.8
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("FL")
                fact: f_energy
                value: fact.value.toFixed(1)
                visible: ui.test || fact.value>0
                valueScale: 0.8
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight/2
            }

            FactValue {
                title: qsTr("WPT")
                fact: f_wpidx
                value: fact.value+1
                visible: ui.test || (m_mode===op_mode_WPT || m_mode===op_mode_STBY)
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("LPS")
                fact: f_loops
                visible: ui.test || (m_mode===op_mode_STBY && fact.value>0)
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("RD")
                fact: f_tdist
                visible: ui.test || isLanding
                property real v: fact.value
                value: (Math.abs(v)<1?0:v.toFixed())+(m_adj>0?"+"+m_adj.toFixed():m_adj<0?"-"+(-m_adj).toFixed():"")
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("LR")
                fact: f_radius
                visible: ui.test || m_mode===op_mode_STBY || m_mode===op_mode_LANDING
                property real v: fact.value
                value: v>=1000?(v/1000).toFixed(1):v.toFixed()
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
            FactValue {
                title: qsTr("AGL")
                fact: f_agl
                value: fact.value.toFixed(1)
                visible: ui.test || m_agl_show
                warning: m_agl_warning
                error: m_agl_failure
                active: m_agl_ready
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
        }
    }
}
