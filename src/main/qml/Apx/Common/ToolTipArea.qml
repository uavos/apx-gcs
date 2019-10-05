import QtQuick 2.12
import QtQuick.Controls 2.12

MouseArea {
    property string text
    anchors.fill: parent
    propagateComposedEvents: true
    hoverEnabled: true
    ToolTip.delay: 1000
    ToolTip.timeout: 5000
    ToolTip.visible: ToolTip.text && containsMouse
    ToolTip.text: text
}
