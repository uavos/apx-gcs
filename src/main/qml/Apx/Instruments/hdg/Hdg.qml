import QtQuick 2.2

import "../common"
import "."

Rectangle {
    id: hdg
    color: "black"

    implicitWidth: 200
    implicitHeight: 400

    clip: true
    property double animation_duration: 100
    property double txtHeight: apx.limit(hdg_deg_rect.height,8,50)
    property bool isLanding:
        m.mode.value===mode_LANDING ||
        m.mode.value===mode_TAKEOFF ||
        m.mode.value===mode_TAXI ||
        (m.mode.value===mode_WPT && m.mtype.value===mtype_line)

    //Component.onDestruction: console.log("delete: "+this)
    HdgImage {
        id: hdg_triangle
        elementName: "hdg-triangle"
        smooth: ui.antialiasing
        anchors.bottom: wheel.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: elementBounds.width*wheel.sf
        height: elementBounds.height*wheel.sf
    }
    Rectangle {
        id: hdg_deg_rect
        anchors.top: parent.top
        anchors.topMargin: 3
        anchors.bottom: hdg_triangle.top
        anchors.bottomMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        width: height*2.5
        border.width: 1
        border.color: "white"
        color: "transparent"
        //radius: 1
        Text {
            id: hdg_text
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: ("00"+apx.angle360(m.yaw.value).toFixed()).slice(-3)
            font.pixelSize: parent.height
            font.family: font_mono
            font.bold: true
            color: "white"

        }
        ToolTipArea {
            text: m.yaw.descr
        }
    }

    Item{
        id: wheel
        anchors.top: parent.top
        anchors.topMargin: parent.height*0.1
        anchors.horizontalCenter: parent.horizontalCenter
        //anchors.verticalCenter: parent.bottom
        //anchors.verticalCenterOffset: -parent.height*0.1
        height: parent.height*(1-0.05-0.2)*2
        width: height
        property double sf: height/wheel_image.elementBounds.height

        HdgImage {
            id: wheel_image
            elementName: "hdg-wheel"
            smooth: ui.antialiasing
            anchors.fill: parent
            rotation: apx.angle(-m.yaw.value)
            Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
        }
        HdgImage {
            id: course_arrow
            elementName: "hdg-course"
            visible: !isLanding
            smooth: ui.antialiasing
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            transform: Rotation{
                origin.x: course_arrow.width/2
                origin.y: course_arrow.height
                angle: apx.angle(-(m.yaw.value-m.course.value))
                Behavior on angle { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }
        }

        HdgImage {
            id: cmd_arrow
            elementName: "hdg-cmd-arrow"
            smooth: ui.antialiasing
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            transform: Rotation{
                origin.x: cmd_arrow.width/2
                origin.y: cmd_arrow.height
                angle: apx.angle(-(m.yaw.value-m.cmd_course.value))
                Behavior on angle { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }
        }

        HdgImage {
            id: center_triangle
            elementName: "hdg-center"
            smooth: ui.antialiasing
            anchors.top: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
        }

        Item {
            id: ils_item
            visible: isLanding
            anchors.fill: parent
            HdgImage {
                id: ils_arrow
                elementName: "ils-arrow"
                smooth: ui.antialiasing
                border: 2
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                width: elementBounds.width*wheel.sf+border*2
                height: elementBounds.height*wheel.sf+border*2
                rotation: apx.angle(-(m.yaw.value-m.tgHDG.value))
                Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
                ToolTipArea {
                    text: m.tgHDG.descr
                }
                HdgImage {
                    id: ils_bar
                    elementName: "ils-bar"
                    smooth: ui.antialiasing
                    border: 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: elementBounds.width*wheel.sf+border*2
                    height: elementBounds.height*wheel.sf+border*2
                    anchors.horizontalCenterOffset: apx.limit(-m.rwDelta.value*width*0.5,-height,height)
                    Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: animation_duration} }
                    ToolTipArea {
                        text: m.rwDelta.descr
                    }
                }

            }

        }
        HdgImage {
            id: wpt_home
            elementName: "hdg-wpt-green"
            smooth: ui.antialiasing
            visible: ui.test || m.dHome.value>5
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            rotation: apx.angle(-(m.yaw.value-m.homeHDG.value))
            Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
        }

        HdgImage {
            elementName: "hdg-wpt-blue"
            smooth: ui.antialiasing
            visible: ui.test || m.dWPT.value>5
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            rotation: apx.angle(-(m.yaw.value-m.wpHDG.value))
            Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
        }


    }

    Item{
        id: hdg_info
        anchors.fill: parent
        anchors.margins: 2

        NumberHdg {
            id: crs_text
            anchors.right: parent.right
            anchors.top: parent.top
            height: hdg.txtHeight
            mfield: m.course
            label: qsTr("CRS")
            text: ("00"+apx.angle360(m.course.value).toFixed()).slice(-3)
            valueColor: "cyan"
        }
        NumberHdg {
            anchors.right: parent.right
            anchors.top: crs_text.bottom
            height: hdg.txtHeight
            mfield: m.cmd_course
            label: ""
            text: ("00"+apx.angle360(m.cmd_course.value).toFixed()).slice(-3)
            valueColor: "magenta"
        }

        NumberHdg {
            id: lat_lon_text
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: apx.limit(hdg.txtHeight*0.5,8,50)
            text: apx.latToString(m.gps_lat.value)+" "+apx.lonToString(m.gps_lon.value)
            valueColor: "gray"
            toolTip: m.gps_lat.descr+", "+m.gps_lon.descr
        }

        NumberHdg {
            id: fuel_text
            property double v: m.fuel.value
            visible: ui.test || v>0
            anchors.right: parent.right
            anchors.bottom: lat_lon_text.top
            height: hdg.txtHeight
            mfield: m.fuel
            label: qsTr("FL")
            text: v>=10?v.toFixed():v.toFixed(1)
        }

        NumberHdg {
            id: frate_text
            property double v: m.frate.value
            visible: ui.test || v>0
            anchors.right: parent.right
            anchors.bottom: fuel_text.top
            height: hdg.txtHeight
            mfield: m.frate
            label: qsTr("FR")
            text: v>=10?v.toFixed():v.toFixed(1)
        }

        NumberHdg {
            id: dh_text
            property double v: (m.mode.value===mode_TAXI)?m.delta.value:m.dHome.value
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            height: hdg.txtHeight
            mfield: m.dHome
            label: qsTr("DH")
            text: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
        }
        NumberHdg {
            id: rd_text
            visible: ui.test || isLanding
            property double v: m.rwDelta.value
            anchors.left: parent.left
            anchors.bottom: dh_text.top
            height: hdg.txtHeight
            mfield: m.rwDelta
            label: qsTr("RD")
            text: m.rwDelta.value.toFixed()+(m.rwAdj.value>0?"+"+m.rwAdj.value.toFixed():m.rwAdj.value<0?"-"+(-m.rwAdj.value).toFixed():"")
        }
        Column{
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: -3

            NumberHdg {
                id: dme_text
                property double v: m.dWPT.value
                smooth: ui.antialiasing
                height: hdg.txtHeight
                mfield: m.dWPT
                label: qsTr("DME")
                text: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            }

            Text {
                id: eta_text
                smooth: ui.antialiasing
                property double valid: m.gSpeed.value>1 && value>0
                property int value: m.ETA.value
                property int tsec: ("0"+Math.floor(value%60)).slice(-2)
                property int tmin: ("0"+Math.floor(value/60)%60).slice(-2)
                property int thrs: Math.floor(value/60/60)
                property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
                height: hdg.txtHeight
                text: valid?sETA:"--:--"
                font.pixelSize: hdg.txtHeight*0.8
                font.family: dme_text.valueFont
                color: dme_text.labelColor
                ToolTipArea { text: m.ETA.descr }
            }

            NumberHdg {
                id: wpt_text
                visible: ui.test || m.mode.value===mode_WPT
                smooth: ui.antialiasing
                height: hdg.txtHeight
                mfield: m.wpidx
                label: qsTr("WPT")
                valueColor: "cyan"
                value: m.wpidx.value+1
            }

            NumberHdg {
                id: poi_text
                visible: ui.test || (m.mode.value===mode_STBY && m.loops.value>0)
                smooth: ui.antialiasing
                height: hdg.txtHeight
                mfield: m.loops
                label: qsTr("LPS")
                valueColor: "cyan"
                value: m.loops.value
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: parent.width/2
        onClicked: {
            if(isLanding) m.rwAdj.setValue(m.rwAdj.value-1)
            else m.cmd_course.setValue(apx.angle(m.cmd_course.value-15))
        }
    }
    MouseArea {
        anchors.fill: parent
        anchors.leftMargin: parent.width/2
        onClicked: {
            if(isLanding) m.rwAdj.setValue(m.rwAdj.value+1)
            else m.cmd_course.setValue(apx.angle(m.cmd_course.value+15))
        }
    }

}
