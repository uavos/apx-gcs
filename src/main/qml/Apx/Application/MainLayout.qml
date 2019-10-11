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
        BoundingRect { item: tools }
        BoundingRect { item: toolsItem }
        BoundingRect { item: leftTop }
        BoundingRect { item: leftBottom }
        BoundingRect { item: bottom }
        BoundingRect { item: bottomLeft }
        BoundingRect { item: bottomRight }
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
        case GroundControl.Layout.ToolBar:
            item.parent=tools
            return true
        case GroundControl.Layout.Tool:
            item.parent=leftTop
            return true
        case GroundControl.Layout.Info:
            item.parent=leftBottom
            return true
        case GroundControl.Layout.Status:
            item.parent=bottomLeft
            item.Layout.alignment=Qt.AlignBottom
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

    RowLayout {
        id: toolsItem
        anchors.left: parent.left
        anchors.top: top.top
        anchors.leftMargin: margins
        anchors.topMargin: vehicles.implicitHeight+margins
        spacing: margins
        MissionTools {
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
        }
        RowLayout {
            id: tools
        }
    }

    RowLayout {
        id: missionList
        anchors.left: parent.left
        anchors.top: toolsItem.bottom
        anchors.bottom: leftBottom.top
        anchors.margins: margins
        MissionListView {
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft
            Layout.fillHeight: true
        }
    }

    RowLayout {
        id: leftBottom
        anchors.left: parent.left
        anchors.bottom: bottom.bottom
        anchors.leftMargin: margins
        anchors.bottomMargin: bottomLeft.implicitHeight+margins
    }

    RowLayout {
        id: leftTop
        anchors.left: missionList.right
        anchors.top: toolsItem.bottom
        anchors.margins: margins
    }

    RowLayout {
        id: bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: margins
        anchors.rightMargin: margins
        anchors.bottomMargin: margins/2

        RowLayout {
            id: bottomLeft
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft|Qt.AlignBottom
        }
        NumbersBar {
            Layout.fillWidth: true
            settingsName: "map"
            defaults: [
                {"bind": "altitude", "title": "ALT", "prec": "0"},
            ]
        }
        RowLayout {
            id: bottomRight
            Layout.alignment: Qt.AlignRight|Qt.AlignBottom
        }
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
            asynchronous: true
            active: apx.vehicles.current.isReplay()
            sourceComponent: Component { TelemetryReader { } }
            visible: status===Loader.Ready
        }
    }
}

