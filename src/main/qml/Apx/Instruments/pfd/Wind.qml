import QtQuick 2.2
import "../common"
import "."

Item {
    id: wind_window
    readonly property bool m_wind: mandala.est.wind.status.value > 0
    readonly property real m_wspd: mandala.est.wind.speed.value
    readonly property real m_whdg: mandala.est.wind.heading.value

    property real value: m_whdg

    property real anumation_duration: 1000
    property bool simplified: false

    visible: m_wind || m_wspd > 0

    readonly property color color: m_wind?"#fff":"yellow"

    PfdImage {
        id: wind_arrow
        anchors.fill: parent
        anchors.rightMargin: parent.width-parent.width*parent.height/parent.width
        anchors.centerIn: parent
        anchors.margins: simplified?1:0
        elementName: simplified?"wind-arrow-simple":"wind-arrow"
        //smooth: ui.antialiasing
        fillMode: Image.PreserveAspectFit
        rotation: value
        Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
    }
    Item {
        anchors.fill: wind_arrow
        rotation: wind_arrow.rotation
        visible: !simplified
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -wind_arrow.height*0.7
            rotation: -parent.rotation
            text: m_wspd.toFixed(m_wspd>=10?0:1)
            color: wind_window.color
            font.family: font_narrow
            font.pixelSize: parent.height*0.5
        }
    }
}
