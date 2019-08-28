import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
//import QtLocation 5.9
import QtPositioning 5.6

import APX.Vehicles 1.0


Item {
    id: control

    readonly property int dotSize: 8

    property var vehicle

    property font font: Qt.application.font
    property bool showDots: true

    property color colorFG: vehicle.active?"#fff":"#aaa"
    property color colorBG: {
        var c="#555"
        if(bGCU) c="#3779C5"
        if(vehicle.streamType===Vehicle.TELEMETRY)c="#377964"
        if(vehicle.streamType===Vehicle.XPDR)c="#376479"
        if(!vehicle.active)c=Qt.darker(c,1.9)
        return c
    }

    //Fact bindings
    property bool bGCU: vehicle.vehicleClass===Vehicle.GCU
    property bool bLOCAL: vehicle.vehicleClass===Vehicle.LOCAL


    //internal
    property string callsign: vehicle.callsign


    property int paddingRight: dotSize+3

    implicitWidth: textLayout.implicitWidth+paddingRight
    implicitHeight: textLayout.implicitHeight

    Connections {
        target: textLayout
        onImplicitWidthChanged: timerWidthUpdate.start()
    }
    property Timer timerWidthUpdate: Timer {
        interval: 1
        onTriggered: {
            implicitWidth=Math.max(textLayout.implicitWidth+paddingRight,implicitWidth)
        }
    }

    //right side info
    ColumnLayout {
        visible: showDots
        spacing: 3
        anchors.fill: parent

        //recording red point
        Rectangle {
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            visible: vehicle.telemetry.active
            border.width: 0
            implicitWidth: dotSize
            implicitHeight: dotSize
            radius: 4
            color: "#C0FF8080"
        }
        //mission available
        Rectangle {
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            visible: vehicle && vehicle.mission.missionSize>0
            border.width: 0
            implicitWidth: dotSize
            implicitHeight: dotSize
            radius: width/2
            color: "#C080FFFF"
        }
        Item {
            Layout.fillHeight: true
        }
    }

    ColumnLayout {
        id: textLayout
        spacing: 0
        Label {
            Layout.minimumWidth: font.pixelSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            font.family: control.font.family
            font.pixelSize: control.font.pixelSize
            font.bold: true
            text: callsign
            color: colorFG
        }
        Label {
            id: infoText
            Layout.fillHeight: true
            Layout.minimumWidth: font.pixelSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            font.family: control.font.family
            font.pixelSize: control.font.pixelSize*0.7
            font.bold: control.font.bold
            text: vehicle.info
            color: colorFG

            onImplicitWidthChanged: timerWidthUpdate.start()
            property Timer timerWidthUpdate: Timer {
                interval: 100
                onTriggered: {
                    infoText.width=Math.max(infoText.width,implicitWidth)
                }
            }
        }
    }
}
