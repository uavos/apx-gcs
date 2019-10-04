import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0

import "qrc:/app"

Item {
    id: control

    /*BoundingRect { anchors.fill: top }
    BoundingRect { anchors.fill: topMission }
    BoundingRect { anchors.fill: leftMission }
    BoundingRect { anchors.fill: left }
    BoundingRect { anchors.fill: bottom }
    BoundingRect { anchors.fill: right }*/

    implicitWidth: 600
    implicitHeight: 500

    readonly property real margins: 10*ui.scale


    function addMainItem(item)
    {
        item.parent=control
        item.anchors.fill=control
        item.z=-1
    }

    function addItem(item, alignment)
    {
        if(alignment&Qt.AlignRight){
            if(alignment&Qt.AlignBottom){
                item.parent=rightBottom
                item.Layout.alignment=Qt.AlignRight
            }else{
                item.parent=rightTop
                item.Layout.alignment=Qt.AlignRight
            }
        }else if(alignment&Qt.AlignLeft){
            if(alignment&Qt.AlignBottom){
                item.parent=bottomLeft
                item.Layout.alignment=Qt.AlignBottom
            }else if(alignment&Qt.AlignVCenter){
                item.parent=left
                item.Layout.alignment=Qt.AlignTop
            }else{
                item.parent=topMission
            }
        }
    }

    RowLayout {
        id: top
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: margins
        RowLayout {
            id: topLeft
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            VehiclesListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
        RowLayout {
            id: topRight
            Layout.alignment: Qt.AlignTop | Qt.AlignRight
            //Layout.fillWidth: false
            //Layout.fillHeight: true
            TaskBar {
                Layout.fillWidth: false
                Layout.fillHeight: false
                //height: 62*ui.scale
                //Layout.alignment: Qt.AlignTop | Qt.AlignRight
            }
        }
    }

    RowLayout {
        id: topMission
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: margins
        anchors.topMargin: topLeft.implicitHeight?topLeft.implicitHeight+margins*2:margins
        MissionTools { }
    }

    RowLayout {
        id: bottom
        anchors.left: bottomLeft.implicitHeight?parent.left:leftMission.right
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.leftMargin: margins
        anchors.bottomMargin: margins
        anchors.rightMargin: rightBottom.implicitWidth?rightBottom.implicitWidth+margins*2:margins
        RowLayout {
            id: bottomLeft
            Layout.alignment: Qt.AlignBottom|Qt.AlignLeft
            Loader {
                id: loaderSignals
                active: showSignals
                sourceComponent: Component { Signals { } }
                visible: active
            }
        }
        NumbersBar {
            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
            layoutDirection: Qt.RightToLeft
            //flow: Flow.TopToBottom
            settingsName: "map"
            defaults: [
                {"bind": "altitude", "title": "ALT", "prec": "0"},
            ]
        }
    }


    ColumnLayout {
        id: leftMission
        anchors.top: topMission.bottom
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.topMargin: margins
        anchors.leftMargin: margins
        anchors.bottomMargin: bottomLeft.implicitHeight?bottomLeft.implicitHeight+margins*2:margins
        Loader {
            id: loaderMission
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.fillHeight: true
            sourceComponent: Component { MissionListView { } }
        }
    }

    ColumnLayout {
        id: left
        anchors.top: topMission.bottom
        anchors.bottom: bottom.top
        anchors.left: parent.left
        anchors.topMargin: margins
        anchors.bottomMargin: margins
        anchors.leftMargin: leftMission.implicitWidth?leftMission.implicitWidth+margins*2:margins
    }

    ColumnLayout {
        id: right
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: margins
        anchors.bottomMargin: margins
        anchors.topMargin: topRight.implicitHeight?topRight.implicitHeight+margins*2:margins
        ColumnLayout {
            id: rightTop
            Layout.alignment: Qt.AlignTop|Qt.AlignRight
            //Layout.fillWidth: false
            //Layout.fillHeight: true
        }
        ColumnLayout {
            id: rightBottom
            Layout.alignment: Qt.AlignBottom|Qt.AlignRight
            //Layout.fillWidth: false
            //Layout.fillHeight: true
        }
    }
}

