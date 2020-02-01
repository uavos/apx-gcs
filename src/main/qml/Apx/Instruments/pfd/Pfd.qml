import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1

import APX.Vehicles 1.0
import "."
import "../common"

Item {
    id: pfd

    readonly property var f_mode: mandala.cmd.op.mode

    readonly property var f_yaw: mandala.est.att.yaw
    readonly property var f_cmd_airspeed: mandala.cmd.air.airspeed
    readonly property var f_cmd_altitude: mandala.cmd.air.altitude

    readonly property var f_ref_status: mandala.est.ref.status

    readonly property var f_gps_su: mandala.sns.gps.su
    readonly property var f_gps_sv: mandala.sns.gps.sv
    readonly property var f_ktas: mandala.est.tecs.ktas
    readonly property var f_ld: mandala.est.tecs.ld
    readonly property var f_stab: mandala.est.tecs.stab

    readonly property var f_thrcut: mandala.cmd.opt.thrcut
    readonly property var f_throvr: mandala.cmd.opt.throvr
    readonly property var f_thr: mandala.ctr.eng.thr
    readonly property var f_rc_thr: mandala.cmd.rc.thr

    readonly property var f_vsys: mandala.sns.pwr.vsys
    readonly property var f_vsrv: mandala.sns.pwr.vsrv
    readonly property var f_vpld: mandala.sns.pwr.vpld
    readonly property var f_veng: mandala.sns.eng.voltage

    readonly property var f_air_temp: mandala.sns.air.temp
    readonly property var f_rt: mandala.sns.aux.rt

    readonly property var f_rpm: mandala.sns.eng.rpm
    readonly property var f_airbrk: mandala.ctr.wing.airbrk

    readonly property var f_status_pwr: mandala.est.status.pwr
    readonly property var f_status_ers: mandala.est.status.ers
    readonly property var f_status_ps: mandala.est.status.ps
    readonly property var f_status_pt: mandala.est.status.pt
    readonly property var f_status_gps: mandala.est.status.gps
    readonly property var f_status_imu: mandala.est.status.imu
    readonly property var f_status_bat: mandala.est.status.bat
    readonly property var f_status_eng: mandala.est.status.eng

    readonly property var f_pwr_servo: mandala.ctr.pwr.servo
    readonly property var f_pwr_payload: mandala.ctr.pwr.payload

    readonly property var f_aux_ers: mandala.sns.aux.ers
    readonly property var f_air_status: mandala.est.air.status

    readonly property var f_ref_altps: mandala.est.ref.altps
    readonly property var f_ref_hmsl: mandala.est.ref.hmsl

    readonly property var f_ctr_hover: mandala.est.ctr.hover
    readonly property var f_rc_ovr: mandala.cmd.rc.ovr
    readonly property var f_starter: mandala.ctr.sw.starter


    clip: true

    implicitWidth: 600
    implicitHeight: 300

    readonly property url pfdImageUrl: Qt.resolvedUrl("pfd.svg")

    property bool showHeading: true
    property bool showWind: true
    property alias flagHeight: pfdScene.flagHeight

    readonly property bool m_err_pwr: f_status_pwr.value > status_pwr_ok

    Rectangle {
        color: "#777"
        border.width: 0
        anchors.fill: parent
    }

    Item {
        id: pfdScene

        width: parent.width
        height: parent.height
        anchors.centerIn: parent

        readonly property real txtHeight: apx.limit(left_window.width*0.2,0,parent.height*0.1)
        readonly property real flagHeight: txtHeight*0.65
        readonly property real topFramesMargin: (width-width*0.6)*0.6*0.2

        readonly property color power_color: (ui.test||m_err_pwr)?"red":"transparent"

        Horizon {
            id: horizon
            margin_left: 0.2
            margin_right: 0.3
            showHeading: pfd.showHeading
        }

        Wind {
            anchors.right: right_window.left
            anchors.bottom: parent.bottom
            anchors.rightMargin: right_window.width*0.2
            anchors.bottomMargin: parent.height*0.2
            width: parent.width*0.05
            height: width
            value: m_whdg-f_yaw.value
        }

        ILS {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: parent.width*(horizon.margin_left-horizon.margin_right)/2
            width: parent.width*0.3
            height: apx.limit(width,0,parent.height*0.6)
        }

        //flight mode text
        Text {
            color: "white"
            anchors.top: parent.top
            anchors.left: parent.horizontalCenter
            anchors.right: right_window.left
            anchors.topMargin: pfdScene.flagHeight*1.5
            text: f_mode.text
            font.pixelSize: pfdScene.txtHeight
            horizontalAlignment: Text.AlignHCenter
            font.family: font_narrow
            ToolTipArea { text: f_mode.descr }
        }



        Rectangle {
            id: left_window
            color: "transparent"
            border.width: 0
            anchors.fill: parent
            anchors.rightMargin: parent.width*0.8
            anchors.leftMargin: 2

            Airspeed {
                id: speed_window
                anchors.fill: parent
                anchors.leftMargin: parent.width*0.6
                anchors.topMargin: pfdScene.topFramesMargin
                anchors.bottomMargin: anchors.topMargin
            }

            RectNum {
                value: f_cmd_airspeed.value.toFixed()
                toolTip: f_cmd_airspeed.descr
                anchors.left: speed_window.left
                anchors.right: speed_window.right
                anchors.top: parent.top
                anchors.bottom: speed_window.top
                anchors.topMargin: 3
                anchors.leftMargin: parent.width*0.1
            }

            Flags {
                id: flags
                txtHeight: pfdScene.flagHeight
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.left: parent.left
                anchors.leftMargin: 1
            }

            Column {
                id: modeFlags
                spacing: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.left: parent.left
                readonly property real modeHeight: pfdScene.flagHeight

                Number {
                    visible: ui.test || value>1 || m_err_pwr
                    height: modeFlags.modeHeight
                    label: "Vs"
                    mfield: f_vsrv
                    precision: 1
                    color: f_pwr_servo.value>0?pfdScene.power_color:"#80000000"
                    blinking: true
                }
                Number {
                    visible: ui.test || value>1 || m_err_pwr
                    height: modeFlags.modeHeight
                    label: "Vm"
                    mfield: f_veng
                    precision: 1
                    color: pfdScene.power_color
                    blinking: true
                }
                Number {
                    visible: ui.test || value>1 || m_err_pwr
                    height: modeFlags.modeHeight
                    label: "Vp"
                    mfield: f_vpld
                    precision: 1
                    color: f_pwr_payload.value>0?pfdScene.power_color:"#80000000"
                    blinking: m_err_pwr
                }

                Rectangle {
                    readonly property bool v: f_status_ers.value > status_ers_ok
                    color: v?"red":"green"
                    border.width: 0
                    radius: 3
                    height: pfdScene.flagHeight
                    width: text.width+3
                    visible: ui.test || f_aux_ers.value > aux_ers_ok || v
                    Text {
                        id: text
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.topMargin: 1
                        text: qsTr("ERS")+" "+f_aux_ers.text
                        color: f_aux_ers.value === aux_ers_disarmed?"white":"yellow"
                        font.pixelSize: parent.height
                        verticalAlignment: Text.AlignVCenter
                        font.family: font_narrow
                    }
                }

                Number {
                    id: at_num
                    height: modeFlags.modeHeight
                    label: "AT"
                    mfield: f_air_temp
                    visible: ui.test
                    onValueChanged: visible=visible || (mfield.value !== 0)
                }
                Number {
                    id: rt_num
                    height: modeFlags.modeHeight
                    label: "RT"
                    mfield: f_rt
                    color: value>=70?"red":value>=50?"#40ffff30":"transparent"
                    blinking: value>=60
                    visible: ui.test
                    onValueChanged: visible=visible || (mfield.value !== 0)
                }

                Number {
                    height: modeFlags.modeHeight
                    label: qsTr("GPS")
                    text: su+"/"+sv
                    toolTip: f_status_gps.descr+", "+f_gps_su.descr+"/"+f_gps_sv.descr
                    readonly property int su: f_gps_su.value
                    readonly property int sv: f_gps_sv.value
                    readonly property bool ref: f_ref_status.value
                    readonly property bool avail: f_status_gps.value===status_gps_ok

                    readonly property bool isOff: (!avail) && (!ref)
                    readonly property bool isErr: ref && (!avail)
                    readonly property bool isOk:  ref && su>4 && su<=sv && (sv/su)<1.8

                    color: isOff?"#80000000":isErr?"red":"transparent"
                    blinking: isErr
                    valueColor: isOk?"white":"yellow"
                }
            }
        }

        Rectangle {
            id: right_window
            color: "transparent"
            border.width: 0
            anchors.fill: parent
            anchors.leftMargin: parent.width*0.7


            Altitude {
                id: altitude_window
                anchors.fill: parent
                anchors.rightMargin: parent.width*0.5
                anchors.topMargin: pfdScene.topFramesMargin
                anchors.bottomMargin: (parent.width-parent.width*0.5)*0.3

                Flag {
                    id: landedFlag
                    anchors.verticalCenterOffset: pfdScene.flagHeight*1.45
                    anchors.centerIn: parent
                    opacity: ui.effects?0.6:1
                    show: ui.test || f_air_status.value > 0
                    visible: show
                    height: pfdScene.flagHeight
                    flagColor: "#8f8"
                    text: f_air_status.text.toUpperCase()
                    toolTip: f_air_status.descr
                }

            }


            Vspeed {
                anchors.left: altitude_window.right
                anchors.right: parent.right
                height: altitude_window.height
                anchors.leftMargin: altitude_window.width*0.05
            }

            RectNum {
                value: f_cmd_altitude.value.toFixed()
                toolTip: f_cmd_altitude.descr
                anchors.left: altitude_window.left
                anchors.right: altitude_window.right
                anchors.top: parent.top
                anchors.bottom: altitude_window.top
                anchors.topMargin: 3
            }
            Row {
                spacing: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2

                Number {
                    visible: ui.test || value>0
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    mfield: f_ref_altps
                    label: qsTr("PS")
                    color: (ui.test || f_status_ps.value>status_ps_ok)?"red":"transparent"
                    blinking: true
                }
                Number {
                    visible: ui.test || value>0
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    mfield: f_ref_hmsl
                    label: qsTr("MSL")
                }
            }
            Number {
                visible: ui.test || value>0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.rightMargin: 4
                height: pfdScene.txtHeight
                mfield: f_ld
                label: qsTr("LD")
                precision: 1
            }

        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 4
            anchors.left: left_window.right
            anchors.leftMargin: 4
            spacing: 4

            Flag {
                id: hoverFlag
                show: f_ctr_hover.value > 0
                visible: opacity
                height: pfdScene.flagHeight
                flagColor: "#8f8"
                text: qsTr("HOVER")
                toolTip: f_ctr_hover.descr
            }
            Flag {
                readonly property real v: f_airbrk.value
                id: airbrkFlag
                show: v > 0
                height: pfdScene.flagHeight
                text: qsTr("AIRBR")
                toolTip: f_airbrk.descr
                Text {
                    readonly property real v: airbrkFlag.v
                    visible: ui.test || (airbrkFlag.show && (v>0) && (v<1))
                    color: "white"
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (v*100).toFixed()
                    font.pixelSize: height
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    font.family: font_narrow
                }
            }
        }
        Column {
            spacing: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            anchors.left: left_window.right
            anchors.leftMargin: 4
            Flag {
                readonly property real v: f_ktas.value
                show: (v!==0) && (v<0.5||v>1.8)
                blinking: false
                height: pfdScene.flagHeight
                flagColor: "yellow"
                text: qsTr("TAS")
                toolTip: f_ktas.descr
            }
            Flag {
                id: flag_CAS
                show: ui.test || f_status_pt.value>status_pt_ok
                blinking: true
                visible: show
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("CAS")
                toolTip: f_status_pt.descr
            }
        }
        Column {
            spacing: 4
            anchors.bottom: parent.verticalCenter
            anchors.bottomMargin: 4
            anchors.horizontalCenter: horizon.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            Flag {
                readonly property real v: f_stab.value
                show: v > 0.5
                blinking: v > 0.8
                height: pfdScene.flagHeight
                flagColor: blinking?"red":"yellow"
                text: qsTr("STALL")
                toolTip: f_stab.descr
            }
            Flag {
                show: f_mode.value<op_mode_UAV && f_status_imu.value > status_imu_ok
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("GYRO")
                toolTip: f_status_imu.descr
            }
            Flag {
                anchors.topMargin: pfdScene.flagHeight*2
                show: f_status_bat.value > status_bat_ok
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("BAT")
                toolTip: f_status_bat.descr
            }
            Flag {
                show: f_rc_ovr.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "yellow"
                text: qsTr("RC")
                toolTip: f_rc_ovr.descr
            }
        }

        Text {
            id: offline
            color: "#80000000"
            anchors.bottom: parent.verticalCenter
            anchors.top: parent.top
            anchors.left: left_window.right
            anchors.right: right_window.left
            text: qsTr("OFFLINE")
            visible: !apx.vehicles.current.isReplay() && !apx.datalink.online
            font.pixelSize: apx.datalink.valid?(parent.height*0.5*0.35):10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
            font.bold: true
        }
        Text {
            id: xpdrData
            color: "#60000000"
            anchors.bottom: parent.verticalCenter
            anchors.left: left_window.right
            anchors.right: right_window.left
            text: apx.vehicles.current.streamType===Vehicle.XPDR?qsTr("XPDR"):qsTr("NO DATA")
            visible: !apx.vehicles.current.isReplay() && apx.datalink.valid && (apx.vehicles.current.streamType!==Vehicle.TELEMETRY)
            font.pixelSize: parent.height*0.5*0.25
            horizontalAlignment: Text.AlignHCenter
            font.family: font_narrow
            font.bold: true
        }

        Item{
            id: center_numbers
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            width: parent.width*0.33
            Number {
                id: rpm_number
                readonly property bool err: f_status_eng.value > status_eng_ok
                visible: ui.test || value>0 || err
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                label: qsTr("RPM")
                value: f_rpm.value/(precision>0?1000:1)
                precision: 0
                toolTip: f_rpm.descr + (precision>0?"[x1000]":"")
                color: (ui.test || err)?"red":"transparent"
                blinking: true
            }
            Flag {
                id: rpm_starter
                anchors.bottom: rpm_number.top
                anchors.bottomMargin: 1
                anchors.left: rpm_number.left
                show: f_starter.value>0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("START")
                toolTip: f_starter.descr
            }


            Number {
                id: thr_number
                visible: true
                anchors.left: parent.horizontalCenter
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                label: qsTr("T")
                readonly property bool throvr: f_throvr.value
                readonly property bool thrcut: f_thrcut.value
                readonly property real thr: f_thr.value
                text: thrcut?qsTr("CUT"):(thr*100).toFixed()
                toolTip: f_thr.descr +"[x100]"+", "+f_thrcut.descr+" ("+qsTr("red")+"), "+f_throvr.descr+" ("+qsTr("blue")+")"
                color: throvr?"blue":thrcut?"red":thr>=0.9?"#80000000":"transparent"
                blinking: thr>=0.98
            }
            Number {
                id: rc_thr_number
                readonly property real rc_thr: f_rc_thr.value
                readonly property bool show: rc_thr>0 && (value!=(f_thr.value*100).toFixed())
                anchors.right: thr_number.left
                anchors.rightMargin: 4
                anchors.top: thr_number.top
                anchors.bottom: thr_number.bottom
                height: pfdScene.txtHeight
                label: qsTr("R")
                opacity: ui.effects?((show||ui.test)?1:0):1
                visible: ui.smooth?opacity:(show||ui.test)
                valueColor: "magenta"
                color: "#80000000"
                value: (rc_thr*100).toFixed()
                toolTip: f_rc_thr.descr +"[x100]"
                Behavior on opacity { enabled: ui.smooth; PropertyAnimation {duration: 500} }
            }

            Number {
                visible: ui.test || value>0 || m_err_pwr
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                mfield: f_vsys
                label: qsTr("V")
                precision: 1
                color: pfdScene.power_color
                blinking: true
            }
        }
    }
}
