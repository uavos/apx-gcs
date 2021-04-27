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
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1

import APX.Vehicles 1.0 as APX
import "."
import "../common"

Item {
    id: pfd

    readonly property APX.Vehicle vehicle: apx.vehicles.current

    readonly property var f_mode: mandala.cmd.proc.mode

    readonly property var f_yaw: mandala.est.att.yaw
    readonly property var f_cmd_airspeed: mandala.cmd.pos.airspeed
    readonly property var f_cmd_altitude: mandala.cmd.pos.altitude

    readonly property var f_gps_src: mandala.sns.gps.src
    readonly property var f_gps_fix: mandala.sns.gps.fix
    readonly property var f_gps_emi: mandala.sns.gps.emi
    readonly property var f_gps_su: mandala.sns.gps.su
    readonly property var f_gps_sv: mandala.sns.gps.sv
    readonly property var f_ref_status: mandala.est.ref.status
    readonly property var f_ahrs_nogps: mandala.cmd.ahrs.nogps


    readonly property var f_ktas: mandala.est.air.ktas
    readonly property var f_ld: mandala.est.air.ld

    readonly property var f_reg_thr: mandala.cmd.reg.eng
    readonly property var f_reg_throvr: mandala.cmd.eng.ovr
    readonly property var f_reg_thrcut: mandala.cmd.eng.cut

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
    readonly property var f_tecs: mandala.cmd.pos.tecs

    readonly property var f_baro_status: mandala.sns.baro.status
    readonly property var f_pwr_status: mandala.sns.pwr.status

    readonly property var f_pitot_status: mandala.sns.pitot.status

    readonly property var f_bat_status: mandala.sns.bat.status

    readonly property var f_eng_status: mandala.sns.eng.status
    readonly property var f_eng_tc: mandala.sns.eng.tc
    readonly property var f_eng_starter: mandala.ctr.eng.starter

    readonly property var f_pwr_servo: mandala.ctr.pwr.servo
    readonly property var f_pwr_payload: mandala.ctr.pwr.payload

    readonly property var f_ers_status: mandala.sns.ers.status
    readonly property var f_ers_block: mandala.sns.ers.block

    readonly property var f_rc_ovr: mandala.cmd.rc.ovr

    readonly property var f_ref_hmsl: mandala.est.ref.hmsl

    readonly property var f_reg_pos: mandala.cmd.reg.pos

    readonly property bool m_reg_spd: mandala.cmd.reg.spd.value
    readonly property bool m_reg_alt: mandala.cmd.reg.alt.value

    // status flags and warnings
    readonly property var f_att_status: mandala.est.att.status
    readonly property var f_att_rest: mandala.est.att.rest
    readonly property var f_pos_status: mandala.est.pos.status
    readonly property var f_pos_hsrc: mandala.est.pos.hsrc
    readonly property var f_lpos_status: mandala.est.lpos.status
    readonly property var f_air_stall: mandala.est.air.stall
    readonly property var f_ahrs_inair: mandala.cmd.ahrs.inair

    readonly property var f_att_valid: mandala.est.att.valid
    readonly property var f_pos_valid: mandala.est.pos.valid
    readonly property var f_gyro_valid: mandala.est.gyro.valid
    readonly property var f_acc_valid: mandala.est.acc.valid

    readonly property bool isValid: f_att_status.value > 0

    clip: true

    implicitWidth: 600
    implicitHeight: 300

    readonly property url pfdImageUrl: Qt.resolvedUrl("pfd.svg")

    property bool showHeading: true
    property bool showWind: true
    property alias flagHeight: pfdScene.flagHeight

    readonly property bool m_err_pwr: f_pwr_status.value > pwr_status_ok

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
            id: windArrow
            anchors.right: right_window.left
            anchors.bottom: parent.bottom
            anchors.rightMargin: right_window.width*0.2
            anchors.bottomMargin: parent.height*0.2
            width: parent.width*0.05
            height: width
            value: m_whdg-f_yaw.value
        }

        // at rest text
        CleanText {
            anchors.bottom: windArrow.top
            anchors.horizontalCenter: windArrow.horizontalCenter
            anchors.bottomMargin: height*2
            height: pfdScene.txtHeight*0.5
            fact: f_att_rest
            type: CleanText.Clean
            show: fact.value > 0
            text: fact.title.toUpperCase()
        }


        ILS {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: parent.width*(horizon.margin_left-horizon.margin_right)/2
            width: parent.width*0.3
            height: apx.limit(width,0,parent.height*0.6)
        }

        //flight mode text
        CleanText {
            id: _modeText
            anchors.top: parent.top
            anchors.left: parent.horizontalCenter
            anchors.right: right_window.left
            anchors.topMargin: pfdScene.flagHeight*1.5
            height: pfdScene.txtHeight*0.7
            fact: f_mode
            type: CleanText.Clean
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
                toolTip: f_cmd_airspeed.title
                anchors.left: speed_window.left
                anchors.right: speed_window.right
                anchors.top: parent.top
                anchors.bottom: speed_window.top
                anchors.topMargin: 3
                anchors.leftMargin: parent.width*0.1
                enabled: m_reg_spd
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

                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_vsrv
                    title: "Vs"
                    show: value>1 || failure
                    precision: 1
                    type_default: f_pwr_servo.value>0?CleanText.Clean:CleanText.Normal
                    failure: m_err_pwr
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_veng
                    title: "Vm"
                    show: value>1 || failure
                    precision: 1
                    failure: m_err_pwr
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_vpld
                    title: "Vp"
                    show: value>1 || failure
                    precision: 1
                    type_default: f_pwr_payload.value>0?CleanText.Clean:CleanText.Normal
                    failure: m_err_pwr
                }

                CleanText {
                    height: pfdScene.flagHeight

                    fact: f_ers_status
                    readonly property int status: f_ers_status.value
                    readonly property bool blocked: f_ers_block.value > 0
                    readonly property bool ok: status == ers_status_ok
                    readonly property bool disarmed: status == ers_status_disarmed

                    visible: ui.test || status > ers_status_ok || blocked

                    type: disarmed
                          ? blocked
                            ? CleanText.Green
                            : CleanText.Black
                    : CleanText.Red
                    prefix: qsTr("ERS")
                }

                NumberText {
                    id: at_num
                    height: modeFlags.modeHeight
                    fact: f_air_temp
                    title: "AT"
                    show: false
                    onValueChanged: show = (show || value !== 0)
                }
                NumberText {
                    id: rt_num
                    height: modeFlags.modeHeight
                    fact: f_rt
                    title: "RT"
                    show: false
                    onValueChanged: show = (show || value !== 0)
                    warning: value>=50
                    failure: value>=70
                    blinking: value>=60
                }

                BlinkingText {
                    height: modeFlags.modeHeight
                    prefix: qsTr("EMI")
                    fact: f_gps_emi

                    readonly property int emi: fact.value

                    visible: ui.test || emi > gps_emi_ok

                    type: (emi === gps_emi_warning)
                          ? CleanText.Normal
                          : CleanText.Red

                    blinking: emi > gps_emi_warning
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_gps_fix
                    title: qsTr("GPS")
                    toolTip: f_gps_fix.title+", "+f_gps_su.title+"/"+f_gps_sv.title

                    readonly property int src: f_gps_src.value
                    readonly property int fix: f_gps_fix.value
                    readonly property int su: f_gps_su.value
                    readonly property int sv: f_gps_sv.value
                    readonly property bool ref: f_ref_status.value === ref_status_initialized
                    readonly property bool avail: fix !== gps_fix_none
                    readonly property bool blocked: f_ahrs_nogps.value > 0

                    readonly property bool isOff: (!avail) && (!ref)
                    readonly property bool isErr: ref && (!avail)
                    readonly property bool isOk:  ref && su>4 && su<=sv && (sv/su)<1.8 && fix >= gps_fix_3D

                    show: isValid || su>0 || sv>0 || fix>0 || src>0

                    type_default: ref?CleanText.Clean:CleanText.Normal
                    failure: isErr
                    warning: blocked

                    textColor: isOk?"white":"yellow"

                    text: su+"/"+sv +(
                              (!ui.test && (!avail || fix === gps_fix_3D))
                              ? ""
                              : (" "+f_gps_fix.text)
                              )
                }
            }

            Row {
                spacing: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.right: parent.right

                StatusFlag {
                    height: pfdScene.flagHeight
                    width: implicitWidth
                    fact: f_pos_status
                    status_warning: pos_status_warning
                    status_show: pos_status_busy
                    status_reset: pos_status_unknown
                    text: qsTr("GPS")
                }
                StatusFlag {
                    height: pfdScene.flagHeight
                    width: implicitWidth
                    fact: f_lpos_status
                    status_warning: lpos_status_warning
                    status_show: lpos_status_busy
                    status_reset: lpos_status_unknown
                    text: qsTr("LPS")
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

                CleanText { // height source
                    anchors.verticalCenterOffset: -pfdScene.flagHeight*1.5
                    anchors.centerIn: parent
                    visible: ui.test || ( fact.value !== pos_hsrc_gps && isValid)
                    height: pfdScene.txtHeight*0.5
                    fact: f_pos_hsrc
                    type: CleanText.Clean
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
                toolTip: f_cmd_altitude.title
                anchors.left: altitude_window.left
                anchors.right: altitude_window.right
                anchors.top: parent.top
                anchors.bottom: altitude_window.top
                anchors.topMargin: 3
                enabled: m_reg_alt
            }
            Row {
                spacing: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2

                NumberText {
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    show: ui.test || value>0
                    fact: f_ref_hmsl
                    title: qsTr("MSL")
                }
            }
            NumberText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.rightMargin: 4
                height: pfdScene.txtHeight
                show: ui.test || value>0
                fact: f_ld
                title: qsTr("LD")
                precision: 1
            }

        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 4
            anchors.left: left_window.right
            anchors.leftMargin: 4
            spacing: 4

            StatusFlag { // pos control mode
                height: pfdScene.flagHeight
                fact: f_reg_pos
                show: ui.test || (status != reg_pos_direct && isValid)
                blinking: false
                text: fact.text
                type: status===reg_pos_off
                      ? CleanText.White
                      : (status===reg_pos_hdg || status===reg_pos_hover)
                        ? CleanText.Yellow
                        : CleanText.Green
            }
            StatusFlag {
                id: airbrkFlag
                height: pfdScene.flagHeight
                fact: f_airbrk
                readonly property real v: fact.value
                show: v > 0
                text: qsTr("AIRBR")
                status_warning: 0.3
                status_failure: 0.7
                status_reset: 0
                CleanText {
                    readonly property real v: airbrkFlag.v
                    fact: airbrkFlag.fact
                    show: (v>0 && v<1)
                    height: pfdScene.flagHeight
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (v*100).toFixed()
                }
            }
            StatusFlag {
                id: tecsFlag
                height: pfdScene.flagHeight
                fact: f_tecs
                readonly property real v: fact.value
                show: v > 0 && v < 1
                text: fact.name.toUpperCase()
                status_reset: 1
                CleanText {
                    readonly property real v: tecsFlag.v
                    fact: tecsFlag.fact
                    show: true
                    height: pfdScene.flagHeight
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (v*100).toFixed()
                }
            }
        }
        // left bottom central
        Column {
            spacing: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            anchors.left: left_window.right
            anchors.leftMargin: 4
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_ktas
                readonly property real v: f_ktas.value
                show: (v!==0) && (v<0.5||v>1.8)
                type: CleanText.Yellow
                text: qsTr("TAS")
            }
            StatusFlag {
                id: flag_CAS
                height: pfdScene.flagHeight
                fact: f_pitot_status
                status_warning: pitot_status_warning
                status_reset: pitot_status_unknown
            }
            StatusFlag { // inair status
                height: pfdScene.flagHeight
                fact: f_ahrs_inair
                text: qsTr("AIR")
                show: ui.test || ( fact.value <= 0 && isValid)
                status_reset: ahrs_inair_yes
            }
        }

        // right bottom central
        Column {
            spacing: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            anchors.right: right_window.left
            anchors.rightMargin: 4
            StatusFlag { // baro status
                height: pfdScene.flagHeight
                fact: f_baro_status
                status_warning: baro_status_warning
                status_reset: baro_status_unknown
            }
        }


        // central
        Column {
            spacing: 4
            anchors.bottom: parent.verticalCenter
            anchors.bottomMargin: 2
            anchors.horizontalCenter: horizon.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_air_stall
                status_warning: air_stall_warning
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_att_status
                status_warning: att_status_warning
                status_show: att_status_busy
                status_reset: att_status_unknown
                text: qsTr("AHRS")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_att_valid
                show: ui.test || (status!==att_valid_yes && f_att_status.value>0)
                failure: true
                text: qsTr("ATT")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_pos_valid
                show: ui.test || (status!==pos_valid_yes && f_pos_status.value>0)
                failure: true
                text: qsTr("POS")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_gyro_valid
                show: ui.test || (status!==gyro_valid_yes && f_att_status.value>0)
                failure: true
                text: qsTr("GYRO")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_acc_valid
                show: ui.test || (status!==acc_valid_yes && f_att_status.value>0)
                failure: true
                text: qsTr("ACC")
            }
            StatusFlag {
                anchors.topMargin: pfdScene.flagHeight*2
                height: pfdScene.flagHeight
                fact: f_bat_status
                status_warning: bat_status_warning
                status_show: bat_status_shutdown
                status_reset: bat_status_unknown
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_rc_ovr
                text: qsTr("RC")
                status_show: rc_ovr_on
                blinking: true
                type: CleanText.Yellow
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
            visible: !vehicle.isReplay && !apx.datalink.online
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
            text: vehicle.text
            visible: !vehicle.isReplay && apx.datalink.valid && (vehicle.streamType!==APX.PVehicle.TELEMETRY)
            font.pixelSize: parent.height*0.5*0.25
            horizontalAlignment: Text.AlignHCenter
            font.family: font_narrow
            font.bold: true
        }

        Item{
            id: center_numbers
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            width: parent.width*0.33
            NumberText {
                id: rpm_number
                readonly property int status: f_eng_status.value
                readonly property bool ok: status > eng_status_unknown
                readonly property bool warn: status == eng_status_warning
                readonly property bool err: status > eng_status_warning
                visible: ui.test || ok || err || value>0
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_rpm
                title: qsTr("RPM")
                value: fact.value/(precision>0?1000:1)
                precision: 0
                toolTip: fact.title + (precision>0?"[x1000]":"")
                warning: warn||!ok
                failure: err
            }
            Column {
                anchors.bottom: rpm_number.top
                anchors.bottomMargin: 1
                anchors.left: rpm_number.left

                BlinkingText { // turbocharger
                    property int v: fact.value
                    visible: ui.test || v > eng_tc_off
                    height: pfdScene.txtHeight*0.7
                    fact: f_eng_tc
                    type: v >= eng_tc_warning
                          ? CleanText.Red
                          : CleanText.Clean
                    blinking: v > eng_tc_warning
                }
                BlinkingText { // engine status
                    property int v: fact.value
                    property bool ctr: f_eng_starter.value>0
                    visible: ui.test || ctr || (v > eng_status_unknown && v < eng_status_running)
                    height: pfdScene.txtHeight*0.7
                    fact: f_eng_status
                    type: ctr
                          ? (v == eng_status_start ? CleanText.Green : CleanText.Red )
                          : CleanText.Clean
                    blinking: ctr
                }
            }

            NumberText {
                id: thr_number
                anchors.left: parent.horizontalCenter
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_thr
                title: qsTr("T")
                readonly property bool thrctr: f_reg_thr.value
                readonly property bool throvr: f_reg_throvr.value
                readonly property bool thrcut: f_reg_thrcut.value
                text: (thrcut && value==0)?qsTr("CUT"):(value*100).toFixed()
                toolTip: fact.title +"[%]"+", "+f_reg_thrcut.title+" ("+qsTr("red")+"), "+f_reg_throvr.title+" ("+qsTr("blue")+"), "+f_reg_thr.title+" ("+qsTr("yellow")+")"
                show: true
                blinking: value>=0.98
                frame: !(thrctr || throvr || thrcut)
                type: throvr
                      ? CleanText.Blue
                      : thrcut
                        ? CleanText.Red
                        : !thrctr
                          ? CleanText.Yellow
                          : value >= 0.9
                            ? CleanText.Normal
                            : CleanText.Clean
            }
            NumberText {
                id: rc_thr_number
                anchors.right: thr_number.left
                anchors.rightMargin: 4
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_rc_thr
                title: qsTr("R")
                text: (value*100).toFixed()
                show: value>0 //&& (value!=(f_thr.value*100).toFixed())
                textColor: "magenta"
                type_default: CleanText.Normal
                toolTip: f_rc_thr.title +"[x100]"
            }

            NumberText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                fact: f_vsys
                title: qsTr("V")
                show: value>0 || failure
                precision: 1
                failure: m_err_pwr
            }
        }
    }
}
