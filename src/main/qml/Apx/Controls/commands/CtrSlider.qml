import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

RowLayout {
    id: control
    property var fact
    property alias title: titleItem.text
    property alias from: ctr.from
    property alias to: ctr.to
    property alias stepSize: ctr.stepSize

    property int size: ctr.implicitHandleHeight*2.2*ui.scale
    property int titleWidth: size*2
    property int valueWidth: titleWidth

    property string toolTip: fact?fact.descr:""

    spacing: 0

    CleanButton {
        id: titleItem
        Layout.minimumWidth: titleWidth
        defaultHeight: control.size
        ui_scale: 1
        color: highlighted?undefined:"#000"
        highlighted: fact.value!==0
        onTriggered: fact.value=0
        toolTip: control.toolTip
    }

    Slider {
        id: ctr
        Layout.fillWidth: true
        Layout.maximumHeight: size

        property real v: fact.value

        from: -1
        to: 1
        stepSize: 0.01

        value: v

        snapMode: Slider.SnapOnRelease

        onMoved: timer.start()
        onVChanged: value=v

        Timer {
            id: timer
            interval: 100
            onTriggered: fact.value=ctr.value
        }
    }

    Label {
        Layout.minimumWidth: valueWidth
        Layout.topMargin: font.pixelSize*0.05+1
        font.family: font_narrow
        font.pixelSize: control.size*0.8
        text: fact.value.toFixed(2)
        horizontalAlignment: Text.AlignRight
    }
}
