import QtQuick 2.2
import "../common"

Flag {
    id: flag
    property bool value: false
    property color color: "yellow"
    height: parent.height
    opacity: ui.effects?0.7:1
    flagColor: (ui.test || value)?color:"#888"
    Behavior on flagColor { enabled: ui.smooth; PropertyAnimation {duration: 200} }
    Behavior on opacity { enabled: ui.smooth; NumberAnimation { duration: 100 } }
    MouseArea {
        id: mouse
        enabled: ui.smooth
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        onEntered: flag.opacity=1
        onExited:  flag.opacity=0.7
    }
}

