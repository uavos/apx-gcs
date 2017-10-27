import QtQuick 2.2
import "../components"

Item {
    id: ils_window
    property double anumation_duration: 1000

    property bool isRW: testUI.value ||
        m.mode.value===mode_LANDING ||
        m.mode.value===mode_TAKEOFF ||
        m.mode.value===mode_TAXI ||
        (m.mode.value===mode_WPT && m.mtype.value===mtype_line)

    property double sz: (width>height?height:width)*0.6

    /*Rectangle { //debug
        color: "#5000ff00"
        border.width: 0
        anchors.fill: parent
    }*/
    PfdImage {
        id: ils_bar_vertical
        visible: testUI.value || m.mode.value===mode_LANDING
        elementName: "ils-bar-vertical"
        fillMode: Image.PreserveAspectFit
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: sz
        width: elementBounds.width*height/elementBounds.height
        smooth: true
        Rectangle {
            antialiasing: true
            color: "#3f3"
            border.width: 0.5
            border.color: "#80000000"
            width: parent.width*1.5
            height: parent.width*0.5
            anchors.centerIn: parent
            anchors.verticalCenterOffset: app.limit(m.delta.value/500*parent.height/2,-parent.height*0.6,parent.height*0.6)
            Behavior on anchors.verticalCenterOffset { enabled: app.settings.smooth.value; PropertyAnimation {duration: anumation_duration; } }
            Text {
                property double value: Math.abs(m.delta.value.toFixed())
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
        visible: isRW
        elementName: "ils-bar-horizontal"
        fillMode: Image.PreserveAspectFit
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: ils_bar_vertical.height
        height: elementBounds.height*width/elementBounds.width
        smooth: true
        Rectangle {
            antialiasing: true
            color: "#3f3"
            border.width: 0.5
            border.color: "#80000000"
            width: parent.height*0.5
            height: parent.height*1.5
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: app.limit(-m.rwDelta.value/20*parent.width/2,-parent.width*0.6,parent.width*0.6)
            Behavior on anchors.horizontalCenterOffset { enabled: app.settings.smooth.value; PropertyAnimation {duration: anumation_duration; } }
            Text {
                property double value: Math.abs(m.rwDelta.value.toFixed())
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
