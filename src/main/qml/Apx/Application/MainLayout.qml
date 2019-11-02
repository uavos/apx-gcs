import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0

import Qt.labs.settings 1.0

Item {
    id: control

    Item {
        z: 9999
        BoundingRect { item: top }
        BoundingRect { item: vehicles }
        BoundingRect { item: taskbar }
        BoundingRect { item: main }
        BoundingRect { item: notifications }
        BoundingRect { item: widgets }
    }

    implicitWidth: 600
    implicitHeight: 500

    readonly property real margins: 10



    property var mainControls: ([])
    property var mainControl

    function add(item, layout, index)
    {
        switch(layout){
        case GroundControl.Layout.Main:
            //item must inherit Control
            addMainControl(item, index)
            return true
        case GroundControl.Layout.MainWidget:
            item.parent=widgets
            item.Layout.alignment=(Qt.AlignRight | Qt.AlignBottom)
            taskbar.addWidget(item)
            return true
        case GroundControl.Layout.Notifications:
            item.parent=notifications
            return true
        }
        return false
    }

    function addMainControl(item, index)
    {
        //mainControls.splice(index, 0, item)
        mainControls.push(item)
        minimizeMainControl(item)
        maximizeMainControl(mainControls[0])
    }
    function maximizeMainControl(item)
    {
        if(mainControl){
            if(mainControl===item)
                return
            minimizeMainControl(mainControl)
        }
        mainControl=item
        item.state="maximized"
        item.parent=control
        item.anchors.fill=control
        item.z=-1
        item.leftPadding=Qt.binding(function(){return main.x})
        item.rightPadding=Qt.binding(function(){return (control.width-main.width-main.x)})
        item.topPadding=Qt.binding(function(){return main.y})
        item.bottomPadding=Qt.binding(function(){return (control.height-main.height-main.y)})
    }
    function minimizeMainControl(item)
    {
        item.state="minimized"
        item.parent=widgets
        item.Layout.alignment=(Qt.AlignRight | Qt.AlignBottom)
        item.anchors.fill=undefined
        item.z=0
        item.leftPadding=0
        item.rightPadding=0
        item.topPadding=0
        item.bottomPadding=0
        item.implicitWidth=200
        item.implicitHeight=200
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
        anchors.rightMargin: widgets.implicitWidth+margins
        anchors.bottomMargin: margins
        anchors.topMargin: vehicles.implicitHeight+margins
    }
    ColumnLayout {
        id: widgets
        anchors.top: top.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: margins
        anchors.bottomMargin: margins
        spacing: margins
        anchors.topMargin: taskbar.height+margins
        Item {
            Layout.fillHeight: true
        }
        clip: true
    }

    ColumnLayout {
        id: notifications
        anchors.top: top.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: margins
        anchors.bottomMargin: margins
        anchors.topMargin: taskbar.height+margins
        spacing: margins
    }
}

