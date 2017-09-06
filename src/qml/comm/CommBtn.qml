import QtQuick 2.2
import "../components"

Flag {
    id: flag
    property bool value: false
    property color color: "yellow"
    height: parent.height
    opacity: mandala.smooth?0.7:1
    flagColor: (mandala.test || value)?color:"#888"
    Behavior on flagColor { PropertyAnimation {duration: mandala.smooth?200:0} }
    Behavior on opacity { NumberAnimation { duration: 100 } }
    MouseArea {
        id: mouse
        enabled: mandala.smooth
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        onEntered: flag.opacity=1
        onExited:  flag.opacity=0.7
    }
}

