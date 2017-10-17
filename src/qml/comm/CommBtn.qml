import QtQuick 2.2
import "../components"

Flag {
    id: flag
    property bool value: false
    property color color: "yellow"
    height: parent.height
    opacity: app.settings.smooth.value?0.7:1
    flagColor: (app.test.value || value)?color:"#888"
    Behavior on flagColor { PropertyAnimation {duration: app.settings.smooth.value?200:0} }
    Behavior on opacity { NumberAnimation { duration: 100 } }
    MouseArea {
        id: mouse
        enabled: app.settings.smooth.value
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        onEntered: flag.opacity=1
        onExited:  flag.opacity=0.7
    }
}

