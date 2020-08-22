import QtQuick 2.2

import "../common"
import "."

Item {

    readonly property int m_mode: mandala.cmd.proc.mode.value
    readonly property int m_man: mandala.cmd.proc.man.value

    readonly property var f_yaw: mandala.est.att.yaw
    readonly property real m_yaw: f_yaw.value
    readonly property var f_course: mandala.est.pos.course
    readonly property real m_course: f_course.value
    readonly property var f_cmd_course: mandala.cmd.pos.course
    readonly property real m_cmd_course: f_cmd_course.value

    readonly property var f_thdg: mandala.est.ctr.thdg
    readonly property real m_thdg: f_thdg.value
    readonly property var f_xtrack: mandala.est.ctr.xtrack
    readonly property real m_xtrack: f_xtrack.value

    //readonly property var f_ref_dist: mandala.est.ref.dist
    //readonly property var f_ref_hdg: mandala.est.ref.hdg

    readonly property var f_wpt_dist: mandala.est.wpt.dist
    readonly property var f_wpt_hdg: mandala.est.wpt.hdg

    readonly property var f_adj: mandala.cmd.proc.adj
    readonly property var f_delta: mandala.est.ctr.delta

    readonly property var f_lat: mandala.est.pos.lat
    readonly property var f_lon: mandala.est.pos.lon

    readonly property var f_speed: mandala.est.pos.speed
    readonly property var f_loops: mandala.est.ctr.loops

    readonly property var f_eta: mandala.est.wpt.eta
    readonly property var f_wpidx: mandala.cmd.proc.wp

    readonly property var f_fuel: mandala.sns.fuel.capacity
    readonly property var f_frate: mandala.sns.fuel.rate


    implicitWidth: 400
    implicitHeight: 400

    property double animation_duration: 100
    property bool isLanding:
        m_mode===proc_mode_LANDING ||
        m_mode===proc_mode_TAKEOFF ||
        m_mode===proc_mode_TAXI || isTrack

    property bool isTrack: m_man===proc_man_track || m_man===proc_man_loiter

    Rectangle {
        id: hdg
        color: "black"

        anchors.centerIn: parent
        height: parent.height
        width: height

        clip: true

        property double txtHeight: apx.limit(hdg_deg_rect.height,8,50)

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
                text: ("00"+apx.angle360(m_yaw).toFixed()).slice(-3)
                font.pixelSize: parent.height
                font.family: font_mono
                font.bold: true
                color: "white"

            }
            ToolTipArea {
                text: f_yaw.descr
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
                rotation: apx.angle(-m_yaw)
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
                    angle: apx.angle(-(m_yaw-m_course))
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
                    angle: apx.angle(-(m_yaw-m_cmd_course))
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
                    rotation: apx.angle(-(m_yaw-m_thdg))
                    Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
                    ToolTipArea {
                        text: f_thdg.descr
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
                        anchors.horizontalCenterOffset: apx.limit(-m_xtrack*width*0.5,-height,height)
                        Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: animation_duration} }
                        ToolTipArea {
                            text: f_xtrack.descr
                        }
                    }

                }

            }
            /*HdgImage {
                id: wpt_home
                elementName: "hdg-wpt-green"
                smooth: ui.antialiasing
                visible: ui.test || f_ref_dist.value>5
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                width: elementBounds.width*wheel.sf
                height: elementBounds.height*wheel.sf
                rotation: apx.angle(-(m_yaw-f_ref_hdg.value))
                Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }*/

            HdgImage {
                elementName: "hdg-wpt-blue"
                smooth: ui.antialiasing
                visible: ui.test || f_wpt_dist.value>5
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                width: elementBounds.width*wheel.sf
                height: elementBounds.height*wheel.sf
                rotation: apx.angle(-(m_yaw-f_wpt_hdg.value))
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
                mfield: f_course
                label: qsTr("CRS")
                text: ("00"+apx.angle360(m_course).toFixed()).slice(-3)
                valueColor: "cyan"
            }
            NumberHdg {
                anchors.right: parent.right
                anchors.top: crs_text.bottom
                height: hdg.txtHeight
                mfield: f_cmd_course
                label: ""
                text: ("00"+apx.angle360(m_cmd_course).toFixed()).slice(-3)
                valueColor: "magenta"
            }

            NumberHdg {
                id: lat_lon_text
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: apx.limit(hdg.txtHeight*0.5,8,50)
                text: apx.latToString(f_lat.value)+" "+apx.lonToString(f_lon.value)
                valueColor: "gray"
                toolTip: f_lat.descr+", "+f_lon.descr
            }

            NumberHdg {
                id: fuel_text
                property double v: f_fuel.value
                visible: ui.test || v>0
                anchors.right: parent.right
                anchors.bottom: lat_lon_text.top
                height: hdg.txtHeight
                mfield: f_fuel
                label: qsTr("FL")
                text: v>=10?v.toFixed():v.toFixed(1)
            }

            NumberHdg {
                id: frate_text
                property double v: f_frate.value
                visible: ui.test || v>0
                anchors.right: parent.right
                anchors.bottom: fuel_text.top
                height: hdg.txtHeight
                mfield: f_frate
                label: qsTr("FR")
                text: v>=10?v.toFixed():v.toFixed(1)
            }

            /*NumberHdg {
                id: dh_text
                property double v: (m_mode===proc_mode_TAXI)?f_delta.value:f_ref_dist.value
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                height: hdg.txtHeight
                mfield: f_ref_dist
                label: qsTr("DH")
                text: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            }*/
            NumberHdg {
                id: rd_text
                visible: ui.test || isLanding
                property double v: m_xtrack
                anchors.left: parent.left
                anchors.bottom: parent.bottom //dh_text.top
                height: hdg.txtHeight
                mfield: f_xtrack
                label: qsTr("RD")
                text: m_xtrack.toFixed()+(f_adj.value>0?"+"+f_adj.value.toFixed():f_adj.value<0?"-"+(-f_adj.value).toFixed():"")
            }
            Column{
                anchors.left: parent.left
                anchors.top: parent.top
                spacing: -3

                NumberHdg {
                    id: dme_text
                    property double v: f_wpt_dist.value
                    smooth: ui.antialiasing
                    height: hdg.txtHeight
                    mfield: f_wpt_dist
                    label: qsTr("DME")
                    text: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
                }

                Text {
                    id: eta_text
                    smooth: ui.antialiasing
                    property double valid: f_speed.value>1 && value>0
                    property int value: f_eta.value
                    property int tsec: ("0"+Math.floor(value%60)).slice(-2)
                    property int tmin: ("0"+Math.floor(value/60)%60).slice(-2)
                    property int thrs: Math.floor(value/60/60)
                    property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
                    height: hdg.txtHeight
                    text: valid?sETA:"--:--"
                    font.pixelSize: hdg.txtHeight*0.8
                    font.family: dme_text.valueFont
                    color: dme_text.labelColor
                    ToolTipArea { text: f_eta.descr }
                }

                NumberHdg {
                    id: wpt_text
                    visible: ui.test || m_mode===proc_mode_WPT
                    smooth: ui.antialiasing
                    height: hdg.txtHeight
                    mfield: f_wpidx
                    label: qsTr("WPT")
                    valueColor: "cyan"
                    value: f_wpidx.value+1
                }

                NumberHdg {
                    id: poi_text
                    visible: ui.test || (m_mode===proc_mode_STBY && f_loops.value>0)
                    smooth: ui.antialiasing
                    height: hdg.txtHeight
                    mfield: f_loops
                    label: qsTr("LPS")
                    valueColor: "cyan"
                    value: f_loops.value
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: parent.width/2
            onClicked: {
                if(isLanding) f_adj.setValue(f_adj.value-1)
                else f_cmd_course.value=apx.angle(m_cmd_course-15)
            }
        }
        MouseArea {
            anchors.fill: parent
            anchors.leftMargin: parent.width/2
            onClicked: {
                if(isLanding) f_adj.setValue(f_adj.value+1)
                else f_cmd_course.value=apx.angle(m_cmd_course+15)
            }
        }

    }
}
