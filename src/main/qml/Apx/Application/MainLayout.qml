import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0

Item {
    id: control

    Item {
        z: 9999
        //visible: false
        BoundingRect { item: top }
        BoundingRect { item: vehicles }
        BoundingRect { item: taskbar }
        BoundingRect { item: main }
        BoundingRect { item: notifications }
    }

    implicitWidth: 600
    implicitHeight: 500

    readonly property real margins: 10

    function add(item, layout)
    {
        switch(layout){
        case GroundControl.Layout.Background:
            item.parent=control
            item.anchors.fill=control
            item.z=-1
            return true
        case GroundControl.Layout.Main:
            item.parent=main
            item.anchors.fill=main
            return true
        case GroundControl.Layout.Notifications:
            item.parent=notifications
            return true
        }
        return false
    }

    RowLayout {
        id: top
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: margins
        VehiclesListView {
            id: vehicles
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        TaskBar {
            id: taskbar
            Layout.alignment: Qt.AlignTop | Qt.AlignRight
            Layout.fillWidth: false
            Layout.fillHeight: false
        }
    }

    Item {
        id: main
        anchors.top: top.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: margins
        anchors.rightMargin: margins
        anchors.bottomMargin: margins
        anchors.topMargin: vehicles.implicitHeight+margins
    }

    ColumnLayout {
        id: notifications
        anchors.top: top.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: margins
        anchors.bottomMargin: margins
        anchors.topMargin: taskbar.height+margins
        Loader {
            Layout.alignment: Qt.AlignRight|Qt.AlignTop
            active: apx.vehicles.current.isReplay()
            visible: active
            sourceComponent: Component { TelemetryReader { } }
        }
    }
}

