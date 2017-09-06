import QtQuick 2.2
import "../components"
import "."

Rectangle {
    id: hdg
    color: "black"
    anchors.fill: parent
    clip: true
    property double anumation_duration: mandala.smooth?100:0
    property double txtHeight: mandala.limit(hdg_deg_rect.height,8,50)
    property bool isLanding:
        mode.value===mode_LANDING ||
        mode.value===mode_TAKEOFF ||
        mode.value===mode_TAXI ||
        (mode.value===mode_WPT && mtype.value===mtype_line)

    HdgImage {
        id: hdg_triangle
        elementName: "hdg-triangle"
        smooth: true
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
            text: ("00"+mandala.angle360(yaw.value).toFixed()).slice(-3)
            font.pixelSize: parent.height
            font.family: font_mono
            font.bold: true
            color: "white"

        }
        ToolTipArea {
            text: yaw.descr
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
            smooth: true
            anchors.fill: parent
            rotation: mandala.angle(-yaw.value)
            Behavior on rotation { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
        }
        HdgImage {
            id: course_arrow
            elementName: "hdg-course"
            visible: !isLanding
            smooth: true
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            transform: Rotation{
                origin.x: course_arrow.width/2
                origin.y: course_arrow.height
                angle: mandala.angle(-(yaw.value-course.value))
                Behavior on angle { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
            }
        }

        HdgImage {
            id: cmd_arrow
            elementName: "hdg-cmd-arrow"
            smooth: true
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            transform: Rotation{
                origin.x: cmd_arrow.width/2
                origin.y: cmd_arrow.height
                angle: mandala.angle(-(yaw.value-cmd_course.value))
                Behavior on angle { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
            }
        }

        HdgImage {
            id: center_triangle
            elementName: "hdg-center"
            smooth: true
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
                smooth: true
                border: 2
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                width: elementBounds.width*wheel.sf+border*2
                height: elementBounds.height*wheel.sf+border*2
                rotation: mandala.angle(-(yaw.value-tgHDG.value))
                Behavior on rotation { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
                ToolTipArea {
                    text: tgHDG.descr
                }
                HdgImage {
                    id: ils_bar
                    elementName: "ils-bar"
                    smooth: true
                    border: 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: elementBounds.width*wheel.sf+border*2
                    height: elementBounds.height*wheel.sf+border*2
                    anchors.horizontalCenterOffset: mandala.limit(-rwDelta.value*width*0.5,-height,height)
                    Behavior on anchors.horizontalCenterOffset { PropertyAnimation {duration: anumation_duration} }
                    ToolTipArea {
                        text: rwDelta.descr
                    }
                }

            }

        }
        HdgImage {
            id: wpt_home
            elementName: "hdg-wpt-green"
            smooth: true
            visible: mandala.test || dHome.value>5
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            rotation: mandala.angle(-(yaw.value-homeHDG.value))
            Behavior on rotation { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
        }

        HdgImage {
            elementName: "hdg-wpt-blue"
            smooth: true
            visible: mandala.test || dWPT.value>5
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: elementBounds.width*wheel.sf
            height: elementBounds.height*wheel.sf
            rotation: mandala.angle(-(yaw.value-wpHDG.value))
            Behavior on rotation { RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
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
            mfield: course
            label: qsTr("CRS")
            text: ("00"+mandala.angle360(course.value).toFixed()).slice(-3)
            valueColor: "cyan"
        }
        NumberHdg {
            anchors.right: parent.right
            anchors.top: crs_text.bottom
            height: hdg.txtHeight
            mfield: cmd_course
            label: ""
            text: ("00"+mandala.angle360(cmd_course.value).toFixed()).slice(-3)
            valueColor: "magenta"
        }

        NumberHdg {
            id: lat_lon_text
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: mandala.limit(hdg.txtHeight*0.5,8,50)
            text: mandala.latToString(gps_lat.value)+" "+mandala.lonToString(gps_lon.value)
            valueColor: "gray"
            toolTip: gps_lat.descr+", "+gps_lon.descr
        }

        NumberHdg {
            id: fuel_text
            property double v: fuel.value
            visible: mandala.test || v>0
            anchors.right: parent.right
            anchors.bottom: lat_lon_text.top
            height: hdg.txtHeight
            mfield: fuel
            label: qsTr("FL")
            text: v>=10?v.toFixed():v.toFixed(1)
        }

        NumberHdg {
            id: frate_text
            property double v: frate.value
            visible: mandala.test || v>0
            anchors.right: parent.right
            anchors.bottom: fuel_text.top
            height: hdg.txtHeight
            mfield: frate
            label: qsTr("FR")
            text: v>=10?v.toFixed():v.toFixed(1)
        }

        NumberHdg {
            id: dh_text
            property double v: (mode.value===mode_TAXI)?delta.value:dHome.value
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            height: hdg.txtHeight
            mfield: dHome
            label: qsTr("DH")
            text: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
        }
        NumberHdg {
            id: rd_text
            visible: mandala.test || isLanding
            property double v: rwDelta.value
            anchors.left: parent.left
            anchors.bottom: dh_text.top
            height: hdg.txtHeight
            mfield: rwDelta
            label: qsTr("RD")
            text: rwDelta.value.toFixed()+(rwAdj.value>0?"+"+rwAdj.value.toFixed():rwAdj.value<0?"-"+(-rwAdj.value).toFixed():"")
        }
        Column{
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: -3

            NumberHdg {
                id: dme_text
                property double v: dWPT.value
                smooth: true
                height: hdg.txtHeight
                mfield: dWPT
                label: qsTr("DME")
                text: v>=1000?(v/1000).toFixed(1)+"km":v.toFixed()
            }

            Text {
                id: eta_text
                smooth: true
                property double valid: gSpeed.value>1 && value>0
                property int value: ETA.value
                property int tsec: ("0"+Math.floor(value%60)).slice(-2)
                property int tmin: ("0"+Math.floor(value/60)%60).slice(-2)
                property int thrs: Math.floor(value/60/60)
                property string sETA: (thrs?thrs+":":"")+("0"+tmin).slice(-2)+":"+("0"+tsec).slice(-2)
                height: hdg.txtHeight
                text: valid?sETA:"--:--"
                font.pixelSize: hdg.txtHeight*0.8
                font.family: dme_text.valueFont
                color: dme_text.labelColor
                ToolTipArea { text: ETA.descr }
            }

            NumberHdg {
                id: wpt_text
                visible: mandala.test || mode.value===mode_WPT
                smooth: true
                height: hdg.txtHeight
                mfield: wpidx
                label: qsTr("WPT")
                valueColor: "cyan"
                value: wpidx.value+1
            }

            NumberHdg {
                id: poi_text
                visible: mandala.test || (mode.value===mode_STBY && loops.value>0)
                smooth: true
                height: hdg.txtHeight
                mfield: loops
                label: qsTr("LPS")
                valueColor: "cyan"
                value: loops.value
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: parent.width/2
        onClicked: {
            if(isLanding) rwAdj.setValue(rwAdj.value-1)
            else cmd_course.setValue(mandala.angle(cmd_course.value-15))
        }
    }
    MouseArea {
        anchors.fill: parent
        anchors.leftMargin: parent.width/2
        onClicked: {
            if(isLanding) rwAdj.setValue(rwAdj.value+1)
            else cmd_course.setValue(mandala.angle(cmd_course.value+15))
        }
    }

}
