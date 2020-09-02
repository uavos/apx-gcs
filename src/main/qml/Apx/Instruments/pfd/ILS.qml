import QtQuick 2.2
import "../common"

Item {
    id: ils_window

    readonly property int m_mode: mandala.cmd.proc.mode.value

    readonly property var f_delta: mandala.est.wpt.delta
    readonly property var f_xtrack: mandala.est.wpt.xtrack

    readonly property int m_reg_pos: mandala.cmd.reg.pos.value
    property bool isTrack: m_reg_pos===reg_pos_track || m_reg_pos===reg_pos_loiter

    property double anumation_duration: 1000

    property bool isLanding: m_mode===proc_mode_LANDING

    property double sz: (width>height?height:width)*0.6

    PfdImage {
        id: ils_bar_vertical
        visible: ui.test || isLanding
        elementName: "ils-bar-vertical"
        fillMode: Image.PreserveAspectFit
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: sz
        width: elementBounds.width*height/elementBounds.height
        //smooth: ui.antialiasing
        Rectangle {
            antialiasing: true
            color: "#3f3"
            border.width: 0.5
            border.color: "#80000000"
            width: parent.width*1.5
            height: parent.width*0.5
            anchors.centerIn: parent
            anchors.verticalCenterOffset: apx.limit(f_delta.value/500*parent.height/2,-parent.height*0.6,parent.height*0.6)
            Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration; } }
            Text {
                property double value: Math.abs(f_delta.value.toFixed())
                visible: value>25
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.left
                text: value
                color: "white"
                font.family: font_narrow
                font.pixelSize: parent.width
            }
        }
    }

    PfdImage {
        id: ils_bar_horizontal
        visible: isTrack
        elementName: "ils-bar-horizontal"
        fillMode: Image.PreserveAspectFit
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: ils_bar_vertical.height
        height: elementBounds.height*width/elementBounds.width
        //smooth: ui.antialiasing
        Rectangle {
            antialiasing: true
            color: "#3f3"
            border.width: 0.5
            border.color: "#80000000"
            width: parent.height*0.5
            height: parent.height*1.5
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: apx.limit(-f_xtrack.value/20*parent.width/2,-parent.width*0.6,parent.width*0.6)
            Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration; } }
            Text {
                property double value: Math.abs(f_xtrack.value.toFixed())
                visible: value>5 && value<100
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                text: value
                color: "white"
                font.family: font_narrow
                font.pixelSize: parent.height
            }
        }
    }


}
