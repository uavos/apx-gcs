import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
//import QtQuick 2.2
//import QtQuick.Controls 1.1
import "."
import "../components"

Item {
    id: pfd
    anchors.fill: parent
    clip: true
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

        property double txtHeight: mandala.limit(left_window.width*0.2,0,parent.height*0.1)
        property double flagHeight: txtHeight*0.65
        property double topFramesMargin: (width-width*0.6)*0.6*0.2

        property color power_color: (mandala.test||error_power.value)?"red":"transparent"

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
            value: windHdg.value-yaw.value
        }

        ILS {
            //anchors.left: left_window.right
            //anchors.right: right_window.left
            //anchors.margins: left_window.width*0.4
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: parent.width*(horizon.margin_left-horizon.margin_right)/2
            width: parent.width*0.3
            height: mandala.limit(width,0,parent.height*0.6)
        }

        //flight mode text
        Text {
            color: "white"
            anchors.top: parent.top
            anchors.left: parent.horizontalCenter
            anchors.right: right_window.left
            anchors.topMargin: pfdScene.flagHeight*1.5
            text: mode.text
            font.pixelSize: pfdScene.txtHeight
            horizontalAlignment: Text.AlignHCenter
            //verticalAlignment: Text.AlignVCenter
            font.family: font_narrow
            ToolTipArea { text: mode.descr }
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
                anchors.topMargin: pfdScene.topFramesMargin //(parent.width-parent.width*0.6)*0.6
                anchors.bottomMargin: anchors.topMargin //(parent.width-parent.width*0.5)*0.3
            }

            RectNum {
                value: cmd_airspeed.value.toFixed()
                toolTip: cmd_airspeed.descr
                anchors.left: speed_window.left
                anchors.right: speed_window.right
                anchors.top: parent.top
                anchors.bottom: speed_window.top
                anchors.topMargin: 3
                anchors.leftMargin: parent.width*0.1
            }

            /*Text {
                anchors.left: speed_window.left
                anchors.right: speed_window.right
                anchors.top: speed_window.bottom
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                text:  airspeed.units
                font.pixelSize: height
                color: "white"
            }*/


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
                    visible: mandala.test || value>1 || error_power.value>0
                    height: modeFlags.modeHeight
                    mfield: Vs
                    precision: 1
                    color: power_servo.value>0?pfdScene.power_color:"#80000000"
                    blinking: true
                }
                Number {
                    visible: mandala.test || value>1 || error_power.value>0
                    height: modeFlags.modeHeight
                    mfield: Vm
                    precision: 1
                    color: pfdScene.power_color
                    blinking: true
                }
                Number {
                    visible: mandala.test || value>1 || error_power.value>0
                    height: modeFlags.modeHeight
                    mfield: Vp
                    precision: 1
                    color: power_payload.value>0?pfdScene.power_color:"#80000000"
                    blinking: error_power.value>0
                }

                Rectangle {
                    color: sb_ers_err.value > 0?"red":"green"
                    border.width: 0
                    radius: 3
                    height: pfdScene.flagHeight
                    width: text.width+3 //left_window.width*0.8
                    visible: mandala.test || sb_ers_disarm.value > 0 || sb_ers_err.value > 0
                    Text {
                        id: text
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.topMargin: 1
                        text: sb_ers_disarm.value>0?qsTr("ERS DISARMED"):qsTr("ERS ERROR")
                        color: sb_ers_disarm.value>0?"white":"yellow"
                        font.pixelSize: parent.height
                        verticalAlignment: Text.AlignVCenter
                        font.family: font_narrow
                    }
                }

                Number {
                    id: at_num
                    height: modeFlags.modeHeight
                    //height: pfdScene.txtHeight
                    mfield: AT
                    visible: false
                    onValueChanged: visible=visible || (mfield.value !== 0)
                }
                Number {
                    id: rt_num
                    height: modeFlags.modeHeight
                    //height: pfdScene.txtHeight
                    mfield: RT
                    color: RT.value>=70?"red":RT.value>=50?"#40ffff30":"transparent"
                    blinking: RT.value>=60
                    visible: false
                    onValueChanged: visible=visible || (mfield.value !== 0)
                }

                Number {
                    //height: pfdScene.txtHeight
                    height: modeFlags.modeHeight
                    label: qsTr("GPS")
                    text: gps_SU.value+"/"+gps_SV.value
                    toolTip: status_gps.descr+", "+gps_SU.descr+"/"+gps_SV.descr
                    property bool isOff: (!status_gps.value) && (!status_home.value)
                    property bool isErr: status_home.value && (!status_gps.value)
                    property bool isOk:  status_home.value && gps_SU.value>4 && gps_SU.value<=gps_SV.value && (gps_SV.value/gps_SU.value)<1.8

                    //color: (mandala.dlcnt>0 || status_gps.value)?(status_gps.value>0?"transparent":"red"):"#80000000"
                    //blinking: mandala.dlcnt>0 && (status_gps.value<=0)
                    //valueColor: (status_home.value && gps_SU.value>4 && (gps_SV.value/gps_SU.value)<1.8)?"white":"yellow"
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
                anchors.topMargin: pfdScene.topFramesMargin //speed_window.anchors.topMargin   //(parent.width-anchors.rightMargin)*0.4
                anchors.bottomMargin: (parent.width-parent.width*0.5)*0.3

                Flag {
                    id: landedFlag
                    //anchors.top: parent.top
                    anchors.verticalCenterOffset: pfdScene.flagHeight*1.45
                    //anchors.right: right_window.left
                    //anchors.rightMargin: 4
                    anchors.centerIn: parent
                    opacity: 0.6
                    show: status_landed.value > 0
                    visible: show
                    height: pfdScene.flagHeight
                    flagColor: "#8f8"
                    text: qsTr("LAND")
                    toolTip: status_landed.descr
                }

            }


            Vspeed {
                anchors.left: altitude_window.right
                anchors.right: parent.right
                height: altitude_window.height
            }

            RectNum {
                value: cmd_altitude.value.toFixed()
                toolTip: cmd_altitude.descr
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
                    visible: mandala.test || value>0
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    mfield: altps_gnd
                    label: qsTr("PS")
                    color: (mandala.test || error_pstatic.value>0)?"red":"transparent"
                    blinking: true
                    MouseArea {
                        anchors.fill: parent
                        enabled: error_pstatic.value
                        onClicked: error_pstatic.setValue(0)
                    }
                }
                Number {
                    visible: mandala.test || value>0
                    anchors.bottom: parent.bottom
                    height: pfdScene.txtHeight
                    mfield: gps_hmsl
                    label: qsTr("MSL")
                }
            }
            Number {
                visible: mandala.test || value>0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.rightMargin: 4
                height: pfdScene.txtHeight
                mfield: ldratio
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
                //anchors.top: parent.top
                //anchors.topMargin: 4
                //anchors.left: left_window.right
                //anchors.leftMargin: 4
                show: cmode_hover.value > 0
                visible: opacity
                //blinking: true
                height: pfdScene.flagHeight
                flagColor: "#8f8"
                text: qsTr("HOVER")
                toolTip: cmode_hover.descr
            }
            Flag {
                id: airbrkFlag
                //anchors.top: hoverFlag.show?hoverFlag.bottom:parent.top
                //anchors.topMargin: 4
                //anchors.left: left_window.right
                //anchors.leftMargin: 4
                show: ctr_airbrk.value > 0
                height: pfdScene.flagHeight
                text: qsTr("AIRBR")
                toolTip: ctr_airbrk.descr
                Text {
                    visible: mandala.test || (airbrkFlag.show && (ctr_airbrk.value>0) && (ctr_airbrk.value<1))
                    color: "white"
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.right
                    anchors.leftMargin: 2
                    text: (ctr_airbrk.value*100).toFixed()
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
                show: (cas2tas.value!==0) && (cas2tas.value<0.5||cas2tas.value>1.8)
                blinking: false
                height: pfdScene.flagHeight
                flagColor: "yellow"
                text: qsTr("TAS")
                toolTip: cas2tas.descr
            }
            Flag {
                id: flag_CAS
                show: mandala.test||error_cas.value > 0
                blinking: true
                visible: show
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("CAS")
                toolTip: error_cas.descr
            }
        }
        Column {
            spacing: 4
            anchors.bottom: parent.verticalCenter
            anchors.bottomMargin: 4
            anchors.horizontalCenter: horizon.horizontalCenter
            anchors.horizontalCenterOffset: horizon.center_shift
            Flag {
                show: stab.value > 0.5
                blinking: stab.value>0.8
                height: pfdScene.flagHeight
                flagColor: blinking?"red":"yellow"
                text: qsTr("STALL")
                toolTip: stab.descr
            }
            Flag {
                show: mode.value<mode_UAV && error_gyro.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("GYRO")
                toolTip: error_gyro.descr
            }
            Flag {
                anchors.topMargin: pfdScene.flagHeight*2
                show: sb_bat_err.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("BAT")
                toolTip: sb_bat_err.descr
            }
            Flag {
                show: status_rc.value > 0
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "yellow"
                text: qsTr("RC")
                toolTip: status_rc.descr
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
            visible: (!mandala.replayData) && (!mandala.online)
            font.pixelSize: mandala.dlcnt>0?(parent.height*0.5*0.35):10
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
            text: mandala.xpdrData?qsTr("XPDR"):qsTr("NO DATA")
            visible: (!mandala.replayData) && mandala.dlcnt>0 && (!mandala.dlinkData)
            font.pixelSize: parent.height*0.5*0.25
            horizontalAlignment: Text.AlignHCenter
            font.family: font_narrow
            font.bold: true
        }
        Text {
            id: replayData
            color: "#40000000"
            anchors.bottom: parent.verticalCenter
            anchors.left: left_window.right
            anchors.right: right_window.left
            text: qsTr("REPLAY")
            visible: mandala.replayData
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
                visible: mandala.test || value>0 || error_rpm.value>0
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                label: qsTr("RPM")
                value: rpm.value/1000
                precision: 1
                toolTip: rpm.descr +"[x1000]"
                color: (mandala.test || error_rpm.value>0)?"red":"transparent"
                blinking: true
                MouseArea {
                    anchors.fill: parent
                    enabled: error_rpm.value
                    onClicked: error_rpm.setValue(0)
                }
            }
            /*Number {
                id: rpm_number
                visible: mandala.test || (value>0 && (!(rpm_error.blink && rpm_error.show)))
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                label: qsTr("RPM")
                value: rpm.value/1000
                precision: 1
                toolTip: rpm.descr +"[x1000]"
            }
            Flag {
                id: rpm_error
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                show: error_rpm.value > 0
                visible: show
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("RPM")
                toolTip: error_rpm.descr
            }*/
            Flag {
                id: rpm_starter
                anchors.bottom: rpm_number.top
                anchors.bottomMargin: 1
                anchors.left: rpm_number.left
                show: sw_starter.value
                blinking: true
                height: pfdScene.flagHeight
                flagColor: "red"
                text: qsTr("START")
                toolTip: sw_starter.descr
            }

            Number {
                id: thr_number
                visible: true //value>0 || cmode_throvr.value || cmode_thrcut.value
                anchors.left: parent.horizontalCenter
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                label: qsTr("T")
                text: cmode_thrcut.value?qsTr("CUT"):(ctr_throttle.value*100).toFixed()
                //valueColor: cmode_thrcut.value?"red":"white"
                toolTip: ctr_throttle.descr +"[x100]"+", "+cmode_thrcut.descr+" ("+qsTr("red")+"), "+cmode_throvr.descr+" ("+qsTr("blue")+")"
                color: cmode_throvr.value?"blue":cmode_thrcut.value?"red":ctr_throttle.value>=0.9?"#80000000":"transparent"
                blinking: ctr_throttle.value>=0.98
                //valueColor: ctr_throttle.value>=0.9?"#822":"white"
            }
            Number {
                id: rc_thr_number
                property bool show: rc_throttle.value && (value!=thr_number.value) //&& (cmode_thrcut.value || (!cmode_throvr.value))
                anchors.right: thr_number.left
                anchors.rightMargin: 4
                anchors.top: thr_number.top
                anchors.bottom: thr_number.bottom
                height: pfdScene.txtHeight
                label: qsTr("R")
                opacity: mandala.smooth?((show||mandala.test)?1:0):1
                visible: mandala.smooth?opacity:(show||mandala.test)
                valueColor: "magenta"
                color: "#80000000"
                value: (rc_throttle.value*100).toFixed()
                toolTip: rc_throttle.descr +"[x100]"
                Behavior on opacity { PropertyAnimation {duration: mandala.smooth?500:0} }
            }

            Number {
                visible: mandala.test || value>0 || error_power.value>0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: pfdScene.txtHeight
                mfield: Ve
                label: qsTr("V")
                precision: 1
                color: pfdScene.power_color
                blinking: true
                MouseArea {
                    anchors.fill: parent
                    enabled: error_power.value
                    onClicked: error_power.setValue(0)
                }
            }
        }
    }
}
