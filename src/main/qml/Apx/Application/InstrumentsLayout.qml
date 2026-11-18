/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import QtCore
import QtQml.Models

import Apx.Common
import Apx.Controls
import Apx.Instruments
import Apx.Application

RowLayout {
    id: control

    function add(item, layout, index)
    {
        switch(layout){
        case GroundControl.Layout.Instrument:
            instrumentsItem.add(item, index)
            if(item.name)
                application.registerUiComponent(item,"instruments."+item.name)
            return true
        }
        return false

    }


    //internal
    readonly property color sepColor: "#244"
    spacing: 0
    height: instrumentsHeight

    // name: the panel is resizable in the layout edit mode
    function addComponent(c, name)
    {
        var obj=c.createObject(control,{"Layout.fillHeight": true})
        separators[name]=c_vsep.createObject(control,{"panel": obj, "name": name})
        return obj
    }
    property var separators: ({})

    // panels follow the order of the layout children
    property bool created: false
    property string appliedOrder: groundControl.panelNames.join() // as created
    readonly property var order: groundControl.layoutOrder
    onOrderChanged: applyOrder()
    function applyOrder()
    {
        if(!created)
            return
        // re-adding a panel breaks the mouse grab of its resize handle
        var key=order.join()
        if(key===appliedOrder)
            return
        appliedOrder=key
        // children are laid out in the order they were added
        var items=[]
        for(var i=0;i<order.length;++i){
            var item=panelItem(order[i])
            var sep=separators[order[i]]
            if(item) items.push(item)
            if(sep) items.push(sep)
        }
        for(i=0;i<items.length;++i)
            items[i].parent=orderHolder
        for(i=0;i<items.length;++i)
            items[i].parent=control
    }
    Item {
        id: orderHolder
        parent: groundControl // not managed by the layout
        visible: false
    }

    readonly property real panelWidthMin: height*0.15
    // width left for the instruments plugins while resizing
    readonly property real instrumentsWidthMin: width*0.1

    property Item statusPanel
    property Item pfdPanel
    property Item numbersPanel
    property Item commandsPanel

    function panelItem(name)
    {
        switch(name){
        case "status": return statusPanel
        case "pfd": return pfdPanel
        case "numbers": return numbersPanel
        case "commands": return commandsPanel
        case "instruments": return instrumentsItem
        }
        return null
    }

    // requested width of a panel: set by the user or the natural one
    function panelWidthRequest(name, naturalWidth)
    {
        var v=groundControl.panelSize(name)
        return v>0?Math.max(panelWidthMin, control.width*v):naturalWidth
    }
    // status and numbers keep the requested width, pfd and commands adapt
    // to any size and are squeezed when the row does not fit the window
    readonly property real panelsWidthFixed:
        panelWidthRequest("status", statusPanel?statusPanel.implicitWidth:0)
        +panelWidthRequest("numbers", numbersPanel?numbersPanel.implicitWidth:0)
    readonly property real panelsWidthFlex:
        panelWidthRequest("pfd", height*2)
        +panelWidthRequest("commands", commandsPanel?commandsPanel.implicitWidth:0)
    readonly property real panelsWidthMax: width-instrumentsWidthMin-panelsWidthFixed-5
    readonly property real panelsFit: (panelsWidthFlex>panelsWidthMax && panelsWidthFlex>0)
                                      ?Math.max(0.1, panelsWidthMax/panelsWidthFlex):1

    function panelWidth(name, naturalWidth)
    {
        var w=panelWidthRequest(name, naturalWidth)
        return (name==="pfd" || name==="commands")?w*panelsFit:w
    }

    Component.onCompleted: {
        statusPanel=addComponent(status, "status")
        pfdPanel=addComponent(pfd, "pfd")
        numbersPanel=addComponent(numbers, "numbers")
        commandsPanel=addComponent(commands, "commands")

        instrumentsItem=addComponent(c_instruments, "instruments")
        created=true
        applyOrder()
    }
    property Item instrumentsItem

    Component {
        id: status
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Status { } }
            Layout.preferredWidth: control.panelWidth("status", implicitWidth)
        }
    }
    Component {
        id: pfd
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Pfd { } }
            Layout.preferredWidth: control.panelWidth("pfd", control.height*2)
        }
    }
    Component {
        id: numbers
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component {
                NumbersBox {
                    settingsName: "instruments"
                    // fill the panel resized by the user
                    fillWidth: groundControl.panelSize("numbers")>0
                }
            }
            Layout.preferredWidth: control.panelWidth("numbers", implicitWidth)
        }
    }
    Component {
        id: commands
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Commands { } }
            Layout.preferredWidth: control.panelWidth("commands", implicitWidth)
        }
    }

    Component {
        id: c_vsep
        Rectangle {
            id: sep
            property Item panel // to the left of the separator
            property string name
            Layout.fillHeight: true
            implicitWidth: 1
            border.width: 0
            color: sepColor
            z: 10
            LayoutHandle {
                // nothing to resize at the window edge
                active: groundControl.layoutEditing && control.order[control.order.length-1]!==sep.name
                property real size0: 0
                // spare width of the instruments plugins block
                property real free0: 0
                // the block is to the right of this separator
                property bool freeRight: false
                // panels to the right, nearest first: [{name, width, size}]
                property var rightPanels: []
                onStarted: {
                    var order=control.order
                    var idx=order.indexOf(sep.name)
                    size0=sep.panel.width
                    free0=Math.max(0, instrumentsItem.width-instrumentsWidthMin)
                    freeRight=order.indexOf("instruments")>idx
                    var r=[]
                    for(var i=idx+1;i<order.length;++i){
                        var n=order[i]
                        if(n==="instruments")
                            continue
                        r.push({"name": n, "width": panelItem(n).width, "size": groundControl.userPanelSize(n)})
                    }
                    rightPanels=r
                }
                onMoved: (delta) => {
                    var sizes={}
                    var p
                    if(sep.name==="instruments"){
                        // the block has no size of its own: resize the next panel
                        if(rightPanels.length<=0)
                            return
                        p=rightPanels[0]
                        sizes[p.name]=apx.limit(p.width-delta, panelWidthMin, p.width+free0)/control.width
                        groundControl.setPanelSizes(sizes)
                        return
                    }
                    // growing takes the space from the instruments plugins first,
                    // then from the panels to the right
                    var free=freeRight?free0:0
                    var d=delta
                    if(d>=0){
                        var avail=free
                        for(var i=0;i<rightPanels.length;++i)
                            avail+=Math.max(0, rightPanels[i].width-panelWidthMin)
                        d=Math.min(d, avail)
                    }else{
                        d=-Math.min(-d, Math.max(0, size0-panelWidthMin))
                    }
                    sizes[sep.name]=(size0+d)/control.width
                    var excess=Math.max(0, d-free)
                    for(i=0;i<rightPanels.length;++i){
                        p=rightPanels[i]
                        var take=Math.min(excess, Math.max(0, p.width-panelWidthMin))
                        excess-=take
                        sizes[p.name]=take>0?(p.width-take)/control.width:p.size
                    }
                    // shrinking gives the space to the next panel,
                    // unless the instruments plugins are there to take it
                    if(d<0 && !freeRight && rightPanels.length>0){
                        p=rightPanels[0]
                        sizes[p.name]=(p.width-d)/control.width
                    }
                    groundControl.setPanelSizes(sizes)
                }
                onReset: if(sep.name!=="instruments") groundControl.setPanelSize(sep.name, 0)
            }
        }
    }

    // Panels drag and drop in the layout edit mode
    Item {
        id: editLayer
        parent: groundControl // not managed by the layout
        x: control.x
        y: control.y
        z: 100
        width: control.width
        height: control.height
        visible: control.visible && groundControl.layoutEditing

        readonly property real gripSize: Math.max(24, Style.buttonSize*1.2)

        property string dragName: ""
        property real dragWidth: 0
        property real dragOffset: 0
        property real dragX: 0
        property int dropIndex: -1
        property real dropX: 0
        property var grab // keeps the image of the dragged panel

        onVisibleChanged: if(!visible) endDrag(false)

        // panels without the dragged one, left to right
        function otherPanels()
        {
            var list=[]
            for(var i=0;i<control.order.length;++i){
                if(control.order[i]!==dragName)
                    list.push(control.order[i])
            }
            return list
        }
        function startDrag(name, mouseX)
        {
            var item=control.panelItem(name)
            if(!item)
                return
            grab=null
            dragWidth=item.width
            dragOffset=mouseX-item.x
            dragX=item.x
            dragName=name
            // the panel is moved as a picture, it is not resized or repainted
            item.grabToImage(function(result){
                if(editLayer.dragName===name)
                    editLayer.grab=result
            })
            moveDrag(mouseX)
        }
        function moveDrag(mouseX)
        {
            if(!dragName)
                return
            dragX=apx.limit(mouseX-dragOffset, -dragWidth/2, width-dragWidth/2)
            var center=dragX+dragWidth/2
            var others=otherPanels()
            var idx=0
            var pos=0
            for(var i=0;i<others.length;++i){
                var item=control.panelItem(others[i])
                if(center>item.x+item.width/2){
                    idx=i+1
                    pos=item.x+item.width
                }
            }
            if(idx===0 && others.length>0)
                pos=control.panelItem(others[0]).x
            dropIndex=idx
            dropX=pos
        }
        function endDrag(apply)
        {
            if(!dragName)
                return
            var name=dragName
            var idx=dropIndex
            var others=otherPanels()
            dragName=""
            dropIndex=-1
            grab=null
            if(!apply || idx<0)
                return
            others.splice(idx, 0, name)
            if(others.join()!==control.order.join())
                groundControl.setPanelOrder(others)
        }

        Repeater {
            model: groundControl.panelNames
            delegate: Item {
                id: block
                readonly property string name: modelData
                readonly property Item panel: control.panelItem(name)
                readonly property bool dragged: editLayer.dragName===name
                x: panel?panel.x:0
                width: panel?panel.width:0
                height: editLayer.height

                // place of the panel being moved
                Rectangle {
                    anchors.fill: parent
                    visible: block.dragged
                    border.width: 0
                    color: "#000"
                    opacity: 0.7
                }
                Rectangle {
                    id: grip
                    anchors.centerIn: parent
                    width: Math.min(editLayer.gripSize, block.width)
                    height: width
                    radius: width/2
                    visible: !block.dragged
                    color: (gripMouse.containsMouse || gripMouse.pressed)
                           ?Material.color(Material.Amber)
                           :"#c0000000"
                    border.width: 1
                    border.color: Material.color(Material.Cyan)
                    MaterialIcon {
                        anchors.centerIn: parent
                        name: "cursor-move"
                        size: grip.width*0.7
                        color: (gripMouse.containsMouse || gripMouse.pressed)?"#000":"#fff"
                    }
                }
                MouseArea {
                    id: gripMouse
                    anchors.fill: grip
                    hoverEnabled: true
                    preventStealing: true
                    cursorShape: pressed?Qt.ClosedHandCursor:Qt.OpenHandCursor
                    onPressed: (event) => editLayer.startDrag(block.name, mapToItem(editLayer, event.x, event.y).x)
                    onPositionChanged: (event) => {
                        if(pressed)
                            editLayer.moveDrag(mapToItem(editLayer, event.x, event.y).x)
                    }
                    onReleased: editLayer.endDrag(true)
                    onCanceled: editLayer.endDrag(false)
                }
            }
        }

        // where the panel is inserted when released
        Rectangle {
            visible: editLayer.dragName!=="" && editLayer.dropIndex>=0
            x: apx.limit(editLayer.dropX-width/2, 0, editLayer.width-width)
            z: 1 // over the lifted panel
            width: 6
            height: parent.height
            border.width: 0
            color: Material.color(Material.Amber)
        }

        // the lifted panel
        Item {
            visible: editLayer.dragName!==""
            x: editLayer.dragX
            y: -height*0.02
            width: editLayer.dragWidth
            height: parent.height
            scale: 1.04
            Rectangle {
                // shadow
                x: parent.width*0.02+4
                y: parent.height*0.04+4
                width: parent.width
                height: parent.height
                border.width: 0
                color: "#000"
                opacity: 0.5
            }
            Rectangle {
                anchors.fill: parent
                border.width: 0
                color: "#000"
            }
            Image {
                anchors.fill: parent
                source: editLayer.grab?editLayer.grab.url:""
                cache: false
            }
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.width: 2
                border.color: Material.color(Material.Amber)
            }
        }
    }

    Component {
        id: c_instruments
        ColumnLayout {
            id: instrumentsView
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 1

            Settings {
                id: settings
                category: "Layout"
                property string instrumentsPlugin: "Terminal"
            }

            function add(plugin, index)
            {
                if(typeof(index)=='undefined')
                    index=pluginsModel.count
                if(!plugin.title)
                    plugin.title=pluginsModel.count+1

                //plugin.parent=control
                plugins.push(plugin)
                var p = {}
                for(var i in plugin) {
                    var v=plugin[i]
                    if(v) p[i]=v
                }
                p.idx = plugins.length-1
                pluginsModel.insert(index, p)

                //show by saved state
                if(plugin.title===settings.instrumentsPlugin)
                    show(plugin)
                else if(view.empty)
                    view.push(plugin)

                //unload on close
                plugin.visible=Qt.binding(function(){return control.visible && view.currentItem===plugin})
            }
            function show(plugin)
            {
                //console.log(plugin)
                if(view.currentItem===plugin)
                    return
                view.replace(plugin)
                settings.instrumentsPlugin=plugin.title
            }
            property var plugins: []
            ListModel {
                id: pluginsModel
            }
            StackView {
                id: view
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
            }
            ListView {
                id: listView
                Layout.alignment: Qt.AlignRight
                Layout.margins: Style.spacing
                implicitWidth: contentWidth
                spacing: Style.spacing
                model: pluginsModel
                visible: count>1
                orientation: ListView.Horizontal
                delegate: ToolButton {
                    property var plugin: plugins[model.idx]
                    text: plugin.title
                    toolTip: plugin.descr
                    iconName: plugin.icon
                    showText: false
                    highlighted: view.currentItem===plugin
                    onTriggered: show(plugin)
                    Component.onCompleted: {
                        listView.implicitHeight=Math.max(listView.implicitHeight, implicitHeight)
                    }
                }
            }
        }
    }
}
