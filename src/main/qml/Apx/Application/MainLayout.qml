import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0

import Qt.labs.settings 1.0

Item {
    id: control

    /*Item {
        z: 9999
        BoundingRect { item: top }
        BoundingRect { item: vehicles }
        BoundingRect { item: taskbar }
        BoundingRect { item: main }
        BoundingRect { item: notifications }
        BoundingRect { item: widgets }
    }*/

    implicitWidth: 600
    implicitHeight: 500

    readonly property real margins: 10



    property var mainPlugins: ([])
    property var mainPlugin

    function add(plugin, layout, index)
    {
        switch(layout){
        case GroundControl.Layout.Main:
            addMainPlugin(plugin, index)
            return true
        case GroundControl.Layout.MainWidget:
            plugin.parent=widgets
            plugin.Layout.alignment=(Qt.AlignRight | Qt.AlignBottom)
            taskbar.addWidget(plugin)
            return true
        case GroundControl.Layout.Notifications:
            plugin.parent=notifications
            return true
        }
        return false
    }

    function addMainPlugin(plugin, index)
    {
        if(!(plugin instanceof Loader)){
            console.error("Plugin must inherit Loader")
        }
        mainPlugins.splice(index, 0, plugin)
        //mainPlugins.push(plugin)
        minimizeMainPlugin(plugin)
        maximizeMainPlugin(mainPlugins[0])
        plugin.loaded.connect(function(){updatePluginPadding(plugin)})
        plugin.visibleChanged.connect(function(){
            if(plugin.visible===false){
                minimizeMainPlugin(plugin)
                if(plugin===mainPlugin){
                    maximizeMainPlugin(mainPlugins[0])
                }
            } else {
                minimizeMainPlugin(plugin)
            }
        })
        plugin.loaded.connect(function(){
            maximizeButtonC.createObject(plugin.item,{"plugin": plugin})
        })

    }
    function maximizeMainPlugin(plugin)
    {
        if(mainPlugin){
            if(mainPlugin===plugin)
                return
            minimizeMainPlugin(mainPlugin)
        }
        mainPlugin=plugin
        //return
        plugin.state="maximized"
        plugin.z=-1
        plugin.parent=control
        plugin.anchors.fill=control
        updatePluginPadding(plugin)
    }
    function minimizeMainPlugin(plugin)
    {
        plugin.state="minimized"
        plugin.z=0
        plugin.anchors.fill=null
        plugin.parent=widgets
        plugin.Layout.alignment=(Qt.AlignRight | Qt.AlignBottom)
        updatePluginPadding(plugin)
    }
    function updatePluginPadding(plugin)
    {
        var item = plugin.item
        if(!item)return
        if(plugin.state==="maximized"){
            item.leftPadding=Qt.binding(function(){return main.x})
            item.rightPadding=Qt.binding(function(){return (control.width-main.width-main.x)})
            item.topPadding=Qt.binding(function(){return main.y})
            item.bottomPadding=Qt.binding(function(){return (control.height-main.height-main.y)})
        }else{
            item.leftPadding=0
            item.rightPadding=0
            item.topPadding=0
            item.bottomPadding=0
            item.implicitWidth=300
            item.implicitHeight=item.implicitWidth*3/4
        }
    }
    Component {
        id: maximizeButtonC
        CleanButton {
            property var plugin
            visible: plugin.state!=="maximized"
            iconName: "fullscreen"
            toolTip: qsTr("Maximize view")
            onTriggered: maximizeMainPlugin(plugin)
        }
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

