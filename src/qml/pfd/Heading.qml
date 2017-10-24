import QtQuick 2.2
import "../components"
import "."

Item {
    id: hdg_window
    //instrument item
    property double anumation_duration: 500
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.leftMargin: 2
    anchors.rightMargin: 2
    height: width*0.09
    property double bottomHeight: height*0.4
    clip: true
    property double value: sys.angle(yaw.value)
    Behavior on value { enabled: app.settings.smooth.value; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }

    /*Rectangle {
        color: "#40000000"
        border.width: 0
        anchors.fill: parent
    }*/

    property double num2scaleWidth: svgRenderer.elementBounds("pfd/pfd.svg", "hdg-scale").width * strip_scale /90
    property double strip_scale: width/svgRenderer.elementBounds("pfd/pfd.svg", "hdg-scale").width
    property double valueShiftMax: width/num2scaleWidth/2

    Item {
        id: scale_top
        anchors.fill: parent
        anchors.bottomMargin: parent.height*0.2
        clip: true

        //horizontal sacle with numbers
        Item {
            id: hdg_scale
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: -sys.angle(value)*num2scaleWidth
            width: parent.width*4*2
            height: parent.height

            //scale
            Repeater {
                model: 4*2
                PfdImage {
                    smooth: true
                    elementName: "hdg-scale"
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: index*width-tickWidth*strip_scale/2
                    width: elementBounds.width*strip_scale
                    height: elementBounds.height*strip_scale
                    property double tickWidth: 2
                }
            }
            //scale numbers
            Repeater {
                model: 36*2
                Text {
                    property int pos: (index-18*2)*10
                    property int num: sys.angle360(pos).toFixed()
                    smooth: true
                    text: num===0?qsTr("N"):
                          num===90?qsTr("E"):
                          num===180?qsTr("S"):
                          num===270?qsTr("W"):
                          ("00"+sys.angle360(pos).toFixed()).slice(-3)
                    //render as image
                    style: Text.Raised
                    styleColor: "transparent"
                    //color: "#A0FFFFFF"
                    color:(num===0||num===90||num===180||num===270)?"#B0FFFFFF":
                          "#80FFFFFF"
                    //color: "white"
                    font.family: font_condenced
                    font.bold: true
                    font.pixelSize: parent.height*0.6
                    //anchors.fill: parent
                    anchors.top: parent.top
                    //anchors.right: parent.right
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: pos*num2scaleWidth
                    //anchors.topMargin: (index-2)*speed_window.num2scaleHeight
                }
            }
        }
        //gps course bug triangle
        PfdImage {
            id: hdg_crs_bug
            elementName: "hdg-crs-bug"
            property double value: sys.angle(course.value-yaw.value)
            Behavior on value { enabled: app.settings.smooth.value; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
            smooth: true
            border: 1
            fillMode: Image.PreserveAspectFit
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: sys.limit(sys.angle(value),-valueShiftMax,valueShiftMax)*num2scaleWidth
            height: bottomHeight
            width: elementBounds.width*height/elementBounds.height
            ToolTipArea { text: course.descr }
        }
        //cmd course bug arrow
        PfdImage {
            id: hdg_cmd_bug
            elementName: "hdg-cmd-bug"
            property double value: sys.angle(cmd_course.value-yaw.value)
            Behavior on value { enabled: app.settings.smooth.value; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
            smooth: true
            border: 1
            fillMode: Image.PreserveAspectFit
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: sys.limit(sys.angle(value),-valueShiftMax,valueShiftMax)*num2scaleWidth
            height: bottomHeight
            width: elementBounds.width*height/elementBounds.height
            ToolTipArea { text: cmd_course.descr }
        }
        //rw hdg bug arrow
        PfdImage {
            id: hdg_rw_bug
            visible:
                mode.value===mode_LANDING ||
                mode.value===mode_TAKEOFF ||
                mode.value===mode_TAXI ||
                (mode.value===mode_WPT && mtype.value===mtype_line)
            elementName: "hdg-rw-bug"
            property double value: sys.angle(tgHDG.value-yaw.value)
            Behavior on value { enabled: app.settings.smooth.value; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
            smooth: true
            border: 1
            fillMode: Image.PreserveAspectFit
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: sys.limit(sys.angle(value),-valueShiftMax,valueShiftMax)*num2scaleWidth
            height: bottomHeight
            width: elementBounds.width*height/elementBounds.height
            ToolTipArea { text: tgHDG.descr }
        }
        //center number box
        PfdImage {
            id: hdg_box
            elementName: "hdg-box"
            smooth: true
            fillMode: Image.PreserveAspectFit
            anchors.topMargin: 1
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height
            width: elementBounds.width*height/elementBounds.height
            Rectangle {
                border.width: 0
                color: "#C0FF0000"
                anchors.fill: hdg_text
                anchors.leftMargin: parent.width*0.05
                anchors.rightMargin: anchors.leftMargin
                anchors.topMargin: anchors.leftMargin+1
                anchors.bottomMargin: parent.height*0.4
                visible: cmode_nomag.value
            }
            Text {
                id: hdg_text
                anchors.fill: parent
                anchors.topMargin: -1
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
                text: ("00"+sys.angle360(value).toFixed()).slice(-3)
                font.pixelSize: parent.height*0.75
                font.family: font_mono
                font.bold: true
                color: cmode_nomag.value?"yellow":"white"

            }
            ToolTipArea { text: yaw.descr }
        }
    }

    //turn rate bar
    PfdImage {
        id: hdg_turnrate
        elementName: "hdg-turnrate"
        smooth: true
        fillMode: Image.PreserveAspectFit
        anchors.fill: parent
        anchors.topMargin: scale_top.height
        property double maxW: 0.95*elementBounds.width*height/elementBounds.height/2
        property double valueW: sys.limit(turn_calc.derivative*num2scaleWidth,-hdg_turnrate.maxW,hdg_turnrate.maxW)
        Behavior on valueW { enabled: app.settings.smooth.value; PropertyAnimation {duration: 500; } }
        //derivative
        Item {
            id: turn_calc
            visible: false
            property double value: yaw.value
            property double derivative: 0
            property double time_s: 0
            property double value_s: 0
            onValueChanged: {
                var t=new Date().getTime();
                var dt=(t-time_s)/1000;
                time_s=t;
                var dv=sys.angle(value-value_s);
                value_s=value;
                if(dt>0.5 || dt<0.01)derivative=0;
                else if(Math.abs(dv>50))derivative=0;
                else derivative=derivative*0.5+(dv/dt)*0.5;
            }
        }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.horizontalCenter
            anchors.topMargin: parent.height*0.15
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            border.color: "white"
            color: "white" //"#C0FF00FF"
            visible: hdg_turnrate.valueW>0
            width: hdg_turnrate.valueW
        }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.horizontalCenter
            anchors.topMargin: parent.height*0.15
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            border.color: "white"
            color: "white" //"#C0FF00FF"
            visible: hdg_turnrate.valueW<0
            width: -hdg_turnrate.valueW
        }
        //steering yaw control
        property double valueR: sys.limit(ctr_rudder.value*hdg_turnrate.maxW,-hdg_turnrate.maxW,hdg_turnrate.maxW)
        Behavior on valueR { enabled: app.settings.smooth.value; PropertyAnimation {duration: 100; } }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.horizontalCenter
            anchors.topMargin: parent.height*0.25
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            color: "#80000000"
            visible: hdg_turnrate.valueR>0
            width: hdg_turnrate.valueR
        }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.horizontalCenter
            anchors.topMargin: parent.height*0.25
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            color: "#80000000"
            visible: hdg_turnrate.valueR<0
            width: -hdg_turnrate.valueR
        }

    }

    property bool isShiftControl:
        mode.value===mode_LANDING ||
        mode.value===mode_TAKEOFF ||
        mode.value===mode_TAXI ||
        (mode.value===mode_WPT && mtype.value===mtype_line) ||
        mode.value===mode_STBY

    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: parent.width/2
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if(isShiftControl) rwAdj.setValue(rwAdj.value-1)
            else cmd_course.setValue(sys.angle(cmd_course.value-15))
        }
    }
    MouseArea {
        anchors.fill: parent
        anchors.leftMargin: parent.width/2
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if(isShiftControl) rwAdj.setValue(rwAdj.value+1)
            else cmd_course.setValue(sys.angle(cmd_course.value+15))
        }
    }
}
