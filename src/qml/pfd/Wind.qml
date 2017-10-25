import QtQuick 2.2
import "../components"

Item {
    id: wind_window
    property double value: m.windHdg.value
    property double anumation_duration: 1000
    property bool simplified: false

    visible: m.windSpd.value>0.5
    //width: simplified?simple_rect.width:height

    /*Rectangle { //debug
        color: "#5000ff00"
        border.width: 0
        anchors.fill: parent
    }*/

    PfdImage {
        id: wind_arrow
        anchors.fill: parent
        anchors.rightMargin: parent.width-parent.width*parent.height/parent.width
        anchors.centerIn: parent
        anchors.margins: simplified?1:0
        elementName: simplified?"wind-arrow-simple":"wind-arrow"
        smooth: true
        fillMode: Image.PreserveAspectFit
        rotation: value
        Behavior on rotation { enabled: app.settings.smooth.value; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
    }
    Item {
        anchors.fill: wind_arrow
        rotation: wind_arrow.rotation
        visible: !simplified
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -wind_arrow.height*0.7
            rotation: -parent.rotation
            text: m.windSpd.value.toFixed(m.windSpd.value>=10?0:1)
            color: "white"
            font.family: font_narrow
            font.pixelSize: parent.height*0.5
        }
    }
    /*Rectangle {
        id: simple_rect
        color: "#80000000"
        border.width: 0
        visible: simplified
        anchors.fill: parent
        anchors.rightMargin: wind_arrow.anchors.rightMargin
        Text {
            id: simple_text
            anchors.left: parent.right
            text: m.windSpd.value.toFixed(m.windSpd.value>=10?0:1)
            color: "white"
            font.family: font_narrow
            font.pixelSize: wind_window.height
        }
    }*/

}
