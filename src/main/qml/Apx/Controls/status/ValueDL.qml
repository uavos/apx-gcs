﻿import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2

import APX.Vehicles 1.0
import Apx.Common 1.0

FactValue {
    id: control
    title: qsTr("DL")
    descr: apx.datalink.descr+":"
           + "\n" + qsTr("Received packets")
           + "\n" + qsTr("Stream errors")
           + "\n" + qsTr("Transmitted packets")

    valueScale: 0.8

    warning: apx.datalink.readonly.value

    enabled: true
    onClicked: m.errcnt=0
    onMenuRequested: apx.datalink.requestMenu()

    readonly property color cGreen: warning?Material.color(Material.Yellow):Material.color(Material.LightGreen)
    readonly property color cRed: warning?Material.color(Material.Yellow):Material.color(Material.DeepOrange)
    readonly property color cYellow: Material.color(Material.Amber)
    readonly property color cCyan: warning?"#fff":Material.color(Material.Cyan)
    readonly property color cGrey: Material.color(Material.Grey)

    Text {
        Layout.maximumHeight: bodyHeight-Layout.topMargin
        Layout.topMargin: font.pixelSize*0.05
        font.family: font_narrow
        font.pixelSize: fontSize(bodyHeight*valueSize)
        verticalAlignment: Text.AlignVCenter
        text: "0%1".arg(apx.datalink.stats.dnlink.cnt.value%100).slice(-2)+" "
        color: apx.datalink.online?(apx.vehicles.current.streamType===Vehicle.TELEMETRY?cGreen:cCyan):cRed
    }
    Text {
        Layout.maximumHeight: bodyHeight-Layout.topMargin
        Layout.topMargin: font.pixelSize*0.05
        font.family: font_narrow
        font.pixelSize: fontSize(bodyHeight*valueSize)
        verticalAlignment: Text.AlignVCenter
        property int value: m.errcnt%1000
        text: value+" "
        color: m.errcnt>1?(errTimer.running?cRed:cYellow):cGrey
        Behavior on color { enabled: ui.smooth; ColorAnimation {duration: 250} }
        Timer {
            id: errTimer
            interval: 5000
            repeat: false
        }
        onValueChanged: errTimer.restart()
    }
    Text {
        Layout.maximumHeight: bodyHeight-Layout.topMargin
        Layout.topMargin: font.pixelSize*0.05
        font.family: font_narrow
        font.pixelSize: fontSize(bodyHeight*valueSize)
        verticalAlignment: Text.AlignVCenter
        text: "0%1".arg(apx.datalink.stats.uplink.cnt.value % 100).slice(-2)+" "
        color: apx.datalink.readonly.value?cRed:cGreen
        //BoundingRect{}
    }
}