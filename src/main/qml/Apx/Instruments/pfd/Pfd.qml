import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1

import APX.Vehicles 1.0
import "."
import "../common"

Item {
    id: pfd

    readonly property var f_mode: mandala.cmd.op.mode
    readonly property int m_mode: f_mode.value

    readonly property var f_yaw: mandala.est.att.yaw
    readonly property var f_cmd_airspeed: mandala.cmd.reg.airspeed
    readonly property var f_cmd_altitude: mandala.cmd.reg.altitude

    readonly property var f_ref_status: mandala.est.ref.status

    readonly property var f_gps_su: mandala.sns.gps.su
    readonly property var f_gps_sv: mandala.sns.gps.sv
    readonly property var f_ktas: mandala.est.calc.ktas

    readonly property var f_thrcut: mandala.cmd.opt.thrcut
    readonly property var f_throvr: mandala.cmd.opt.throvr
    readonly property var f_thr: mandala.ctr.eng.thr
    readonly property var f_rc_thr: mandala.cmd.rc.thr


    clip: true

    implicitWidth: 600
    implicitHeight: 300

    readonly property url pfdImageUrl: Qt.resolvedUrl("pfd.svg")

    property bool showHeading: true
    property bool showWind: true
    property alias flagHeight: pfdScene.flagHeight

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

        property double txtHeight: apx.limit(left_window.width*0.2,0,parent.height*0.1)
        property double flagHeight: txtHeight*0.65
        property double topFramesMargin: (width-width*0.6)*0.6*0.2

        property color power_color: (ui.test||m.error_power.value)?"red":"transparent"

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
                property double modeHeight: pfdScene.flagHeight

                Number {
                    visible: ui.test || value>1 || m.error_power.value>0
                    height: modeFlags.modeHeight
                    mfield: m.Vs
                    precision: 1
                    color: m.power_servo.value>0?pfdScene.power_color:"#80000000"
                    blinking: true
                }
                Number {
                    visible: ui.test || value>1 || m.error_power.value>0
                    height: modeFlags.modeHeight
                    mfield: m.Vm
                    precision: 1
                    color: pfdScene.power_color
                    blinking: true
                }
                Number {
                    visible: ui.test || value>1 || m.error_power.value>0
                    height: modeFlags.modeHeight
                    mfield: m.Vp
                    precision: 1
                    color: m.power_payload.value>0?pfdScene.power_color:"#80000000"
                    blinking: m.error_power.value>0
                }

                Rectangle {
                    color: m.sb_ers_err.value > 0?"red":"green"
                    border.width: 0
                    radius: 3
                    height: pfdScene.flagHeight
                    width: text.width+3
                    visible: ui.test || m.sb_ers_disarm.value > 0 || m.sb_ers_err.value > 0
                    Text {
                        id: text
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.topMargin: 1
                        text: m.sb_ers_disarm.value>0?qsTr("ERS DISARMED"):qsTr("ERS ERROR")
                        color: m.sb_ers_disarm.value>0?"white":"yellow"
                        font.pixelSize: parent.height
                        verticalAlignment: Text.AlignVCenter
                        font.family: font_narrow
                    }
                }

                Number {
                    id: at_num
                    height: modeFlags.modeHeight
                    mfield: m.AT
                    visible: false
                    onValueChanged: visible=visible || (mfield.value !== 0)
                }
                Number {
                    id: rt_num
                    height: modeFlags.modeHeight
                    mfield: m.RT
                    color: m.RT.value>=70?"red":m.RT.value>=50?"#40ffff30":"transparent"
                    blinking: m.RT.value>=60
                    visible: false
                    onValueChanged: visible=visible || (mfield.value !== 0)
                }

                Number {
                    height: modeFlags.modeHeight
                    label: qsTr("GPS")
                    text: su+"/"+sv
                    toolTip: m.status_gps.descr+", "+f_gps_su.descr+"/"+f_gps_sv.descr
                    readonly property int su: f_gps_su.value
                    readonly property int sv: f_gps_sv.value
                    readonly property bool ref: f_ref_status.value
                    property bool isOff: (!m.status_gps.value) && (!ref)
                    property bool isErr: ref && (!m.status_gps.value)
                    property bool isOk:  ref && su>4 && su<=sv && (sv/su)<1.8

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
                    show: m.status_landed.value > 0
                    visible: show
                    height: pfdScene.flagHeight
                    flagColor: "#8f8"
                    text: qsTr("LAND")
                    toolTip: m.status_landed.descr
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
                    mfield: m.altps_gnd
                    label: qsTr("PS")
                    color: (ui.test || m.error_pstatic.value>0)?"red":"transparent"
                    blinking: true
                    MouseArea {
                        anchors.fill: parent
                        enabled: m.error_pstatic.value
                        onClicked: m.error_pstatic.setValue(0)
                    }
                }
                Number {
                    visible: ui.test || value>0
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    mfield: m.gps_hmsl
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
                mfield: m.ldratio
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
                show: m.cmode_hover.value > 0
                visible: opacity
                height: pfdScene.flagHeight
                flagColor: "#8f8"
                text: qsTr("HOVER")
                toolTip: m.cmode_hover.descr
            }
            Flag {
                id: airbrkFlag
                show: m.ctr_airbrk.value > 0
                height: pfdScene.flagHeight
                text: qsTr("AIRBR")
                toolTip: m.ctr_airbrk.descr
                Text {
                    visible: ui.test || (airbrkFlag.show && (m.ctr_airbrk.value>0) && (m.ctr_airbrk.value<1))
                    color: "white"
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (m.ctr_airbrk.value*100).toFixed()
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
                property real v: f_ktas.value
                show: (v!==0) && (v<0.5||v>1.8)
                blinking: false
                height: pfdScene.flagHeight
                flagColor: "yellow"
                text: qsTr("TAS")
                toolTip: f_ktas.descr
            }
            Flag {
                id: flag_CAS
                show: ui.test||m.error_cas.value > 0
                blinking: true
                visible: show
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("CAS")
                toolTip: m.error_cas.descr
            }
        }
        Column {
            spacing: 4
            anchors.bottom: parent.verticalCenter
            anchors.bottomMargin: 4
            anchors.horizontalCenter: horizon.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            Flag {
                show: m.stab.value > 0.5
                blinking: m.stab.value>0.8
                height: pfdScene.flagHeight
                flagColor: blinking?"red":"yellow"
                text: qsTr("STALL")
                toolTip: m.stab.descr
            }
            Flag {
                show: m_mode<op_mode_UAV && m.error_gyro.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("GYRO")
                toolTip: m.error_gyro.descr
            }
            Flag {
                anchors.topMargin: pfdScene.flagHeight*2
                show: m.sb_bat_err.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("BAT")
                toolTip: m.sb_bat_err.descr
            }
            Flag {
                show: m.status_rc.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "yellow"
                text: qsTr("RC")
                toolTip: m.status_rc.descr
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
                visible: ui.test || value>0 || m.error_rpm.value>0
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                label: qsTr("RPM")
                value: m.rpm.value/(precision>0?1000:1)
                precision: 0
                toolTip: m.rpm.descr + (precision>0?"[x1000]":"")
                color: (ui.test || m.error_rpm.value>0)?"red":"transparent"
                blinking: true
                MouseArea {
                    anchors.fill: parent
                    enabled: m.error_rpm.value
                    onClicked: m.error_rpm.setValue(0)
                }
            }
            Flag {
                id: rpm_starter
                anchors.bottom: rpm_number.top
                anchors.bottomMargin: 1
                anchors.left: rpm_number.left
                show: m.sw_starter.value
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("START")
                toolTip: m.sw_starter.descr
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
                property bool show: rc_thr>0 && (value!=(f_thr.value*100).toFixed())
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
                visible: ui.test || value>0 || m.error_power.value>0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                mfield: m.Ve
                label: qsTr("V")
                precision: 1
                color: pfdScene.power_color
                blinking: true
                MouseArea {
                    anchors.fill: parent
                    enabled: m.error_power.value
                    onClicked: m.error_power.setValue(0)
                }
            }
        }
    }
}
