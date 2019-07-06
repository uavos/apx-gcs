import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
//import QtLocation 5.9
import QtPositioning 5.6

import APX.Vehicles 1.0

RowLayout {
    id: control
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


    spacing: 3

    ColumnLayout {
        spacing: 0
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: height
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
            Layout.fillHeight: true
            Layout.minimumWidth: font.pixelSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            font.family: control.font.family
            font.pixelSize: control.font.pixelSize*0.7
            font.bold: control.font.bold
            text: vehicle.info
            color: colorFG
        }
    }

    //right side info
    ColumnLayout {
        visible: showDots
        Layout.fillWidth: false
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignTop
        //anchors.right: parent.right
        //anchors.top: parent.top
        //anchors.bottom: parent.bottom
        //anchors.margins: 1
        spacing: 3

        //recording red point
        Rectangle {
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            visible: vehicle.telemetry.active
            border.width: 0
            implicitWidth: radius*2
            implicitHeight: radius*2
            radius: 4
            color: "#C0FF8080"
        }
        //mission available
        Rectangle {
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            visible: vehicle.mission.missionSize>0
            border.width: 0
            implicitWidth: radius*2
            implicitHeight: radius*2
            radius: 4
            color: "#C080FFFF"
        }
    }
}
