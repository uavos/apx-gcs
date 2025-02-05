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
import QtQuick.Controls
import QtQuick.Controls.Material

import APX.Fleet as APX
import "."
import "../common"

Item {
    id: pfd

    readonly property APX.Unit unit: apx.fleet.current

    readonly property var f_mode: mandala.fact("cmd.proc.mode")

    readonly property var f_yaw: mandala.fact("est.att.yaw")
    readonly property var f_cmd_airspeed: mandala.fact("cmd.pos.airspeed")
    readonly property var f_cmd_altitude: mandala.fact("cmd.pos.altitude")

    readonly property var f_gps_src: mandala.fact("sns.gps.src")
    readonly property var f_gps_fix: mandala.fact("sns.gps.fix")
    readonly property var f_gps_emi: mandala.fact("sns.gps.emi")
    readonly property var f_gps_su: mandala.fact("sns.gps.su")
    readonly property var f_gps_sv: mandala.fact("sns.gps.sv")
    readonly property var f_ref_status: mandala.fact("est.ref.status")
    readonly property var f_ins_nogps: mandala.fact("cmd.ins.nogps")


    readonly property var f_ktas: mandala.fact("est.air.ktas")
    readonly property var f_ld: mandala.fact("est.air.ld")

    readonly property var f_reg_thr: mandala.fact("cmd.reg.eng")
    readonly property var f_reg_throvr: mandala.fact("cmd.eng.ovr")
    readonly property var f_reg_thrcut: mandala.fact("cmd.eng.cut")

    readonly property var f_thr: mandala.fact("ctr.eng.thr")
    readonly property var f_rc_thr: mandala.fact("cmd.rc.thr")

    readonly property var f_vsys: mandala.fact("sns.pwr.vsys")
    readonly property var f_vsrv: mandala.fact("sns.pwr.vsrv")
    readonly property var f_vpld: mandala.fact("sns.pwr.vpld")
    readonly property var f_veng: mandala.fact("sns.eng.voltage")

    readonly property var f_air_temp: mandala.fact("sns.air.temp")
    readonly property var f_rt: mandala.fact("sns.aux.rt")

    readonly property var f_rpm: mandala.fact("est.eng.rpm")
    readonly property var f_airbrk: mandala.fact("ctr.wing.airbrk")
    readonly property var f_reg_airbrk: mandala.fact("cmd.reg.airbrk")
    readonly property var f_tecs: mandala.fact("cmd.pos.tecs")

    readonly property var f_baro_status: mandala.fact("sns.baro.status")
    readonly property var f_pwr_status: mandala.fact("sns.pwr.status")

    readonly property var f_pitot_status: mandala.fact("sns.pitot.status")

    readonly property var f_bat_status: mandala.fact("sns.bat.status")

    readonly property var f_eng_status: mandala.fact("est.eng.status")
    readonly property var f_eng_mode: mandala.fact("cmd.eng.mode")
    readonly property var f_eng_tc: mandala.fact("sns.eng.tc")
    readonly property var f_eng_starter: mandala.fact("ctr.eng.starter")

    readonly property var f_pwr_servo: mandala.fact("ctr.pwr.servo")
    readonly property var f_pwr_payload: mandala.fact("ctr.pwr.payload")

    readonly property var f_ers_status: mandala.fact("sns.ers.status")
    readonly property var f_ers_block: mandala.fact("sns.ers.block")

    readonly property var f_rc_mode: mandala.fact("cmd.rc.mode")

    readonly property var f_ref_hmsl: mandala.fact("est.ref.hmsl")

    readonly property var f_reg_hdg: mandala.fact("cmd.reg.hdg")
    readonly property var f_reg_hover: mandala.fact("cmd.reg.hover")
    readonly property var f_wpt_status: mandala.fact("est.wpt.status")

    readonly property var f_reg_spd: mandala.fact("cmd.reg.spd")
    readonly property var f_reg_alt: mandala.fact("cmd.reg.alt")

    // status flags and warnings
    readonly property var f_att_status: mandala.fact("est.att.status")
    readonly property var f_ins_rest: mandala.fact("est.ins.rest")
    readonly property var f_pos_status: mandala.fact("est.pos.status")
    readonly property var f_ins_href: mandala.fact("est.ins.href")
    readonly property var f_lpos_status: mandala.fact("est.lpos.status")
    readonly property var f_air_stall: mandala.fact("est.air.stall")
    readonly property var f_ins_inair: mandala.fact("cmd.ins.inair")
    readonly property var f_ins_hsel: mandala.fact("cmd.ins.hsel")

    readonly property var f_att_valid: mandala.fact("est.att.valid")
    readonly property var f_pos_valid: mandala.fact("est.pos.valid")
    readonly property var f_gyro_valid: mandala.fact("est.gyro.valid")
    readonly property var f_acc_valid: mandala.fact("est.acc.valid")

    readonly property bool isValid: f_att_status.value > 0

    opacity: ui.effects?((apx.datalink.valid && !(unit.streamType===APX.PUnit.XPDR||unit.streamType===APX.PUnit.TELEMETRY))?0.7:1):1

    clip: true

    implicitWidth: 600
    implicitHeight: 300

    readonly property url pfdImageUrl: Qt.resolvedUrl("pfd.svg")

    property bool showHeading: true
    property bool showWind: true
    property alias flagHeight: pfdScene.flagHeight

    readonly property bool m_err_pwr: f_pwr_status.value > f_pwr_status.eval.ok

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

        readonly property real txtHeight: Math.min(left_window.width*0.21,parent.height*0.102)
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
            fact: f_ins_rest
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
                enabled: f_reg_spd.value > 0
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
                spacing: pfdScene.txtHeight/8
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
                    readonly property bool ok: status == f_ers_status.eval.ok
                    readonly property bool disarmed: status == f_ers_status.eval.disarmed

                    visible: ui.test || status > f_ers_status.eval.ok || blocked

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

                    visible: ui.test || emi > f_gps_emi.eval.ok

                    type: (emi == f_gps_emi.eval.warning)
                          ? CleanText.Normal
                          : CleanText.Red

                    blinking: emi > f_gps_emi.eval.warning
                }
                NumberText {
                    height: modeFlags.modeHeight
                    fact: f_gps_fix
                    title: qsTr("GPS")
                    toolTip: f_gps_fix.title+", "+f_gps_su.title+"/"+f_gps_sv.title

                    readonly property int src: f_gps_src.value
                    readonly property int fix: f_gps_fix.value
                    readonly property int fix_valid: f_gps_fix.everReceived
                    readonly property int su: f_gps_su.value
                    readonly property int sv: f_gps_sv.value
                    readonly property bool ref: f_ref_status.value == f_ref_status.eval.initialized
                    readonly property bool avail: fix !== f_gps_fix.eval.none
                    readonly property bool blocked: f_ins_nogps.value > 0

                    readonly property bool isOff: (!avail) && (!ref)
                    readonly property bool isErr: ref && (!avail) && fix_valid
                    readonly property bool isOk:  ref && su>4 && su<=sv && (sv/su)<1.8 && (fix >= f_gps_fix.eval['3D'] || !fix_valid)

                    show: isValid || su>0 || sv>0 || fix>0 || src>0

                    type_default: ref?CleanText.Clean:CleanText.Normal
                    failure: isErr
                    warning: blocked

                    textColor: isOk?"white":"yellow"

                    text: su+"/"+sv +(
                              (!ui.test && (!avail || fix == f_gps_fix.eval['3D']))
                              ? ""
                              : (" "+f_gps_fix.text)
                              )
                }
            }

            Row {
                spacing: pfdScene.txtHeight/10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.right: parent.right

                StatusFlag {
                    height: pfdScene.flagHeight
                    width: implicitWidth
                    fact: f_pos_status
                    status_warning: Number(f_pos_status.eval.warning)
                    status_show: Number(f_pos_status.eval.busy)
                    status_reset: Number(f_pos_status.eval.unknown)
                    text: qsTr("GPS")
                }
                StatusFlag {
                    height: pfdScene.flagHeight
                    width: implicitWidth
                    fact: f_lpos_status
                    status_warning: Number(f_lpos_status.eval.warning)
                    status_show: Number(f_lpos_status.eval.busy)
                    status_reset: Number(f_lpos_status.eval.unknown)
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

                CleanText { // height reference
                    id: ins_href
                    anchors.verticalCenterOffset: -pfdScene.flagHeight*1.5
                    anchors.centerIn: parent
                    visible: ui.test || (isValid)
                    height: pfdScene.txtHeight*0.5
                    fact: f_ins_href
                    type: CleanText.Clean
                }
                CleanText { // height source selection
                    anchors.horizontalCenter: ins_href.horizontalCenter
                    anchors.bottom: ins_href.top
                    anchors.bottomMargin: height/4
                    visible: ins_href.visible && f_ins_href.text!=f_ins_hsel.text
                    height: ins_href.height
                    fact: f_ins_hsel
                    type: CleanText.Yellow
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
                enabled: f_reg_alt.value > 0
            }
            Row {
                spacing: pfdScene.txtHeight/10
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
            anchors.topMargin: spacing
            anchors.left: left_window.right
            anchors.leftMargin: spacing
            spacing: pfdScene.txtHeight/8

            StatusFlag { // hdg control mode
                height: pfdScene.flagHeight
                fact: f_reg_hdg
                readonly property bool off_but_hover: !status && hoverFlag.status
                readonly property bool always_hidden: status == f_reg_hdg.eval.direct && f_wpt_status.value > 0 && !hoverFlag.status
                show: ui.test || (!always_hidden && !off_but_hover && isValid)
                blinking: false
                text: fact.text==="runway" ? "rw" : fact.text
                type: status == f_reg_hdg.eval.off || !f_wpt_status.value
                      ? CleanText.White
                      : CleanText.Green
            }
            StatusFlag { // hover mode
                id: hoverFlag
                height: pfdScene.flagHeight
                fact: f_reg_hover
                show: ui.test || (status && isValid)
                blinking: status == f_reg_hover.eval.vel
                text: blinking?"HVEL":"HOVER"
                type: (!f_wpt_status.value)
                      ? CleanText.White
                      : blinking
                        ? CleanText.Yellow
                        : CleanText.Green
            }
            StatusFlag {
                id: airbrkFlag
                height: pfdScene.flagHeight
                fact: f_airbrk
                readonly property real v: fact.value
                readonly property int reg_mode: f_reg_airbrk.value
                show: v > 0 || reg_mode > 0
                text: qsTr("ABR")
                status_warning: 0.3
                status_failure: 0.7
                status_reset: 0
                CleanText {
                    readonly property real v: airbrkFlag.v
                    readonly property int reg_mode: airbrkFlag.reg_mode
                    fact: airbrkFlag.fact
                    show: (v>0 && v<1) || reg_mode > 0
                    height: pfdScene.flagHeight
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (reg_mode > 0?f_reg_airbrk.text[0]+" ":"") + (v*100).toFixed()
                }
            }
            StatusFlag {
                id: tecsFlag
                height: pfdScene.flagHeight
                fact: f_tecs
                property bool controlled: false
                readonly property real v: fact.value
                show: (controlled||v > 0) && v < 1
                text: fact.name.toUpperCase()
                status_reset: 1
                onVChanged: if(v>0 && v<1) controlled=true
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
            spacing: pfdScene.txtHeight/8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: spacing
            anchors.left: left_window.right
            anchors.leftMargin: spacing
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
                status_warning: Number(f_pitot_status.eval.warning)
                status_reset: Number(f_pitot_status.eval.unknown)
            }
            StatusFlag { // inair status
                height: pfdScene.flagHeight
                fact: f_ins_inair
                text: qsTr("AIR")
                show: ui.test || ( fact.value != f_ins_inair.eval.yes && isValid)
                status_reset: Number(f_ins_inair.eval.yes)
                status_warning: Number(f_ins_inair.eval.hover)
            }
        }

        // right bottom central
        Column {
            spacing: pfdScene.txtHeight/8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            anchors.right: right_window.left
            anchors.rightMargin: 4
            StatusFlag { // baro status
                height: pfdScene.flagHeight
                fact: f_baro_status
                status_warning: Number(f_baro_status.eval.warning)
                status_reset: Number(f_baro_status.eval.unknown)
            }
        }


        // central
        Column {
            spacing: pfdScene.txtHeight/8
            anchors.verticalCenter: parent.verticalCenter
            // anchors.bottomMargin: 2
            anchors.horizontalCenter: horizon.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_air_stall
                status_warning: Number(f_air_stall.eval.warning)
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_att_status
                status_warning: Number(f_att_status.eval.warning)
                status_show: Number(f_att_status.eval.busy)
                status_reset: Number(f_att_status.eval.unknown)
                text: qsTr("INS")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_att_valid
                show: ui.test || (status != f_att_valid.eval.yes && f_att_status.value>0)
                failure: true
                text: qsTr("ATT")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_pos_valid
                show: ui.test || (status != f_pos_valid.eval.yes && f_pos_status.value>0)
                failure: true
                text: qsTr("POS")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_gyro_valid
                show: ui.test || (status != f_gyro_valid.eval.yes && f_att_status.value>0)
                failure: true
                text: qsTr("GYRO")
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_acc_valid
                show: ui.test || (status != f_acc_valid.eval.yes && f_att_status.value>0)
                failure: true
                text: qsTr("ACC")
            }
            StatusFlag {
                anchors.topMargin: pfdScene.flagHeight*2
                height: pfdScene.flagHeight
                fact: f_bat_status
                status_warning: Number(f_bat_status.eval.warning)
                status_show: Number(f_bat_status.eval.shutdown)
                status_reset: Number(f_bat_status.eval.unknown)
            }
            StatusFlag {
                height: pfdScene.flagHeight
                fact: f_rc_mode
                text: qsTr("RC")
                status_show: Number(f_rc_mode.eval.manual)
                blinking: true
                type: CleanText.Yellow
            }
        }

        Text {
            color: "#80000000"
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            text: qsTr("DISCONNECTED")
            visible: !unit.isReplay && !apx.datalink.online
            font: apx.font_narrow(apx.datalink.valid?(parent.height*0.5*0.35):10,true)
        }
        Text {
            color: "#60000000"
            anchors.top: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            text: unit.text
            visible: !unit.isReplay && apx.datalink.valid && (unit.streamType!==APX.PUnit.TELEMETRY)
            horizontalAlignment: Text.AlignHCenter
            font: apx.font_narrow(parent.height*0.5*0.25,true)
            Text {
                color: "#70000000"
                anchors.top: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: unit.protocol?unit.protocol.telemetry.text:""
                font: apx.font_narrow(parent.font.pixelSize*0.5,true)
            }
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
                readonly property bool ok: status > f_eng_status.eval.idle
                readonly property bool warn: status == f_eng_status.eval.warning
                readonly property bool err: status > f_eng_status.eval.warning
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
                    visible: ui.test || v > f_eng_tc.eval.off
                    height: pfdScene.txtHeight*0.7
                    fact: f_eng_tc
                    type: v >= f_eng_tc.eval.warning
                          ? CleanText.Red
                          : CleanText.Clean
                    blinking: v > f_eng_tc.eval.warning
                }
                BlinkingText { // engine cmd mode
                    property int mode: fact.value
                    height: pfdScene.txtHeight*0.7
                    fact: f_eng_mode
                    readonly property bool starter: f_eng_starter.value>0
                    readonly property int status: f_eng_status.value
                    blinking: starter
                    visible: ui.test 
                        || starter 
                        || mode >= f_eng_mode.eval.start 
                        || (status != f_eng_status.eval.ok && mode > f_eng_mode.eval.auto)

                    type: starter
                          ? (mode == f_eng_mode.eval.start ? CleanText.Green : CleanText.Red )
                          : status >= f_eng_status.eval.warning || (status != f_eng_status.eval.ok && mode > f_eng_mode.eval.auto)
                            ? CleanText.Red
                            : CleanText.Clean
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
                bold: thrcut
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
