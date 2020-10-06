import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Button {
    id: control

    property string toolTip

    property var color

    property int size: 32 * ui_scale

    property int minimumWidth: 0

    property real ui_scale: ui.scale

    signal triggered()
    signal activated()

    focus: false
    font.capitalization: Font.MixedCase

    // geometry

    padding: height/32
    spacing: height/20
    topInset: 0
    bottomInset: 1

    leftPadding: padding+1
    rightPadding: padding+2
    topPadding: padding
    bottomPadding: padding+2

    implicitHeight: size
    implicitWidth: defaultWidth

    property int defaultWidth: Math.max(height, Math.max(minimumWidth, implicitContentWidth + leftPadding+rightPadding))

    // colors
    highlighted: activeFocus
    Material.background: color ? color : undefined
    Material.theme: Material.Dark
    Material.accent: Material.color(Material.BlueGrey)
    Material.primary: Material.color(Material.LightGreen)

    // tooltip

    ToolTip.delay: 1000
    ToolTip.timeout: 5000
    ToolTip.visible: ToolTip.text && (down || hovered)
    ToolTip.text: toolTip


    // actions

    onClicked: triggered()


    onCheckedChanged: if(checked) activated()

    onActiveFocusChanged: focusTimer.start()

    Timer {
        id: focusTimer
        interval: 100
        onTriggered: {
            if(control.pressed) start()
            else control.focus=false
        }
    }
}
