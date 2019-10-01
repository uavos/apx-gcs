import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

RowLayout {
    id: control
    property var fact
    property alias title: titleItem.text

    property real min: -100
    property real max: 100
    property real stepSize: 1
    property int precision: 0
    property real mult: 1

    property int size: buttonHeight
    property int titleWidth: size*1.8
    property int valueWidth: size

    spacing: 4

    property real value: fact.value*mult

    CleanButton {
        id: titleItem
        Layout.minimumWidth: titleWidth
        defaultHeight: control.size
        ui_scale: 1
        color: highlighted?undefined:"#000"
        highlighted: value!==0
        onTriggered: fact.value=0
        toolTip: qsTr("Reset")+" "+fact.descr
    }

    Label {
        Layout.minimumWidth: valueWidth
        Layout.topMargin: font.pixelSize*0.05+1
        font.family: font_narrow
        font.pixelSize: control.size*0.8
        text: value.toFixed(precision)
        horizontalAlignment: Text.AlignHCenter
        Rectangle {
            x: -parent.x
            y: -parent.y
            width: control.width-1
            height: control.height-1
            border.width: 1
            border.color: "#222"
            color: "transparent"
            radius: 6
        }
    }

    CleanButton {
        defaultHeight: control.size
        ui_scale: 1
        text: "-"
        enabled: value>min
        onTriggered: adjust(-stepSize)
        onPressAndHold: {
            timer.step=-stepSize
            timer.start()
        }
        onReleased: timer.stop()
        toolTip: qsTr("Decrease")+" "+fact.descr
    }
    CleanButton {
        defaultHeight: control.size
        ui_scale: 1
        text: "+"
        enabled: value<max
        onTriggered: adjust(stepSize)
        onPressAndHold: {
            timer.step=stepSize
            timer.start()
        }
        onReleased: timer.stop()
        toolTip: qsTr("Increase")+" "+fact.descr
    }
    Timer {
        id: timer
        property real step: 0
        interval: 100
        repeat: true
        onTriggered: if(!adjust(step))stop()
    }

    function adjust(v)
    {
        var x=value+v
        if(x>=max)x=max
        else if(x<min)x=min
        else {
            fact.value=x/mult
            return 1
        }
        fact.value=x/mult
        return 0
    }
}
