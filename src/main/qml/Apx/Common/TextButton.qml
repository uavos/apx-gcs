import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Button {
    id: control

    property var color

    property color textColor: Material.primaryTextColor
    property color disabledTextColor: Material.hintTextColor

    property int textAlignment: Text.AlignHCenter
    property int size: 64

    property int minimumWidth: 0

    property real ui_scale: ui.scale

    signal triggered()
    signal activated()

    focus: false

    // geometry

    padding: 2*ui_scale
    leftPadding: padding+1
    rightPadding: padding+2
    topPadding: padding
    bottomPadding: padding+1
    spacing: 3*ui_scale

    topInset: 0
    bottomInset: 1

    background.y: 0
    background.width: width
    background.height: height-1

    implicitHeight: size
    implicitWidth: defaultWidth

    property int defaultWidth: Math.max(height, Math.max(minimumWidth, contentItem.implicitWidth + leftPadding+rightPadding))

    // colors

    highlighted: activeFocus

    Material.background: color?color:undefined //"#80303030"

    Material.theme: Material.Dark
    Material.accent: Material.color(Material.BlueGrey)
    Material.primary: Material.color(Material.LightGreen)


    font.pixelSize: Math.max(6, height*0.8-2)

    contentItem: Text {
        font: control.font
        text: control.text
        color: control.textColor

        textFormat: Text.PlainText
        horizontalAlignment: control.textAlignment
        verticalAlignment: Text.AlignVCenter

        //implicitWidth: contentWidth
        //implicitHeight: contentHeight
    }

    onClicked: {
        triggered()
    }

    onCheckedChanged: if(checked)activated()

    onActiveFocusChanged: {
        focusTimer.start()
    }
    Timer {
        id: focusTimer
        interval: 100
        onTriggered: {
            if(control.pressed)start()
            else control.focus=false
        }
    }
}
