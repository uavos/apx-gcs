import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.13

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




    ListModel {
        id: pluginsModel
    }
    property var plugins: []

    function add(plugin, layout, index)
    {
        switch(layout){
        case GroundControl.Layout.Main:
            addPluginFrame(plugin, index)
            if(plugin.state!=="maximized"){
                taskbar.addWidgetControl(plugin)
            }
            return true
        case GroundControl.Layout.Notifications:
            plugin.parent=notifications
            return true
        }
        return false
    }

    property var mainPlugin

    function maximizePlugin(plugin)
    {
        if(!plugin){
            //ensure at least one plugin maximized
            if(mainPlugin) return
            for(var i=0;i<pluginsModel.count;++i){
                var p=plugins[pluginsModel.get(i).idx]
                if(!p.active)continue
                if(!p.state)continue
                maximizePlugin(p)
                break
            }
            return
        }
        if(mainPlugin){
            if(mainPlugin===plugin)
                return
            mainPlugin.state="minimized"
        }
        mainPlugin=plugin
        if(plugin.state) plugin.state="maximized"
        plugin.z=-1
        plugin.parent=control
        plugin.anchors.fill=control
        updatePluginPadding(plugin)
    }
    function minimizePlugin(plugin)
    {
        if(plugin!==mainPlugin) return
        mainPlugin=null

        if(plugin.state){
            for(var i=0;i<(pluginsModel.count-1);++i){
                var p=plugins[pluginsModel.get(i).idx]
                if(p!==plugin)continue
                pluginsModel.move(i,pluginsModel.count-1,1)
                break
            }
        }
        if(plugin.state) plugin.state="minimized"
        plugin.z=0
        updatePluginPadding(plugin)
    }
    function updatePluginState(plugin)
    {
        if(plugin.state==="maximized"){
            maximizePlugin(plugin)
        }else{
            minimizePlugin(plugin)
        }
    }
    function updatePluginPadding(plugin)
    {
        var item = plugin.item
        if(!item) return
        if(typeof(item.leftPadding)=="undefined") return
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
        }
    }

    function addPluginFrame(plugin, index)
    {
        plugins.push(plugin)
        var p = {}
        for(var i in plugin) p[i]=plugin[i]
        p.idx = plugins.length-1
        if(index<pluginsModel.count) pluginsModel.insert(index, p)
        else pluginsModel.append(p)

        plugin.loaded.connect(function(){ updatePluginPadding(plugin) })
        plugin.stateChanged.connect(function(){ updatePluginState(plugin) })
        plugin.activeChanged.connect(function(){
            if(plugin.active)return
            minimizePlugin(plugin)
            maximizePlugin()
        })
        updatePluginState(plugin)
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
        anchors.rightMargin: widgets.implicitWidth+margins*2
        anchors.bottomMargin: margins
        anchors.topMargin: vehicles.implicitHeight+margins
    }
    ColumnLayout {
        id: widgets
        anchors.top: top.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: margins
        anchors.leftMargin: margins
        anchors.bottomMargin: margins
        spacing: margins
        anchors.topMargin: taskbar.height+margins
        Item {
            Layout.fillHeight: true
        }
        Repeater {
            model: pluginsModel
            delegate: MainPluginFrame {
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                plugin: plugins[idx]
                //onMaximize: maximizePlugin(plugin)
                //onMinimize: minimizePlugin(plugin)
            }
        }
        //clip: true
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

