import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0

import "qrc:/app"

Item {
    id: control

    BoundingRect { anchors.fill: top }
    BoundingRect { anchors.fill: top2 }
    BoundingRect { anchors.fill: left }
    BoundingRect { anchors.fill: bottom }
    BoundingRect { anchors.fill: right }

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
            }else{
                item.parent=rightTop
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
            //Layout.fillWidth: true
            //Layout.fillHeight: true
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
        id: top2
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: margins
        anchors.topMargin: topLeft.implicitHeight?topLeft.implicitHeight+margins*2:margins
        MissionTools { }
    }

    RowLayout {
        id: bottom
        anchors.left: bottomLeft.implicitHeight?parent.left:left.right
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.leftMargin: margins
        anchors.bottomMargin: margins
        anchors.rightMargin: rightBottom.implicitWidth?rightBottom.implicitWidth+margins*2:margins
        RowLayout {
            id: bottomLeft
            Layout.alignment: Qt.AlignBottom|Qt.AlignLeft
            //Layout.fillWidth: true
            //Layout.fillHeight: true
            Loader {
                id: loaderSignals
                active: showSignals
                //Layout.alignment: Qt.AlignBottom | Qt.AlignLeft
                sourceComponent: Component { Signals { } }
                visible: active
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1000
        }
        RowLayout {
            id: bottomRight
            Layout.alignment: Qt.AlignBottom|Qt.AlignRight
            //Layout.fillHeight: true
            NumbersBar {
                //Layout.alignment: Qt.AlignBottom|Qt.AlignRight
                //Layout.fillWidth: true
                //layoutDirection: Qt.RightToLeft
                settingsName: "map"
                defaults: [
                    {"bind": "altitude", "title": "ALT", "prec": "0"},
                ]
            }
        }
    }


    ColumnLayout {
        id: left
        anchors.top: top2.bottom
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

