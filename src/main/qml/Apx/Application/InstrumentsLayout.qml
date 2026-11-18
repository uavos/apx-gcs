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
        c_vsep.createObject(control,{"panel": obj, "name": name?name:""})
        return obj
    }

    readonly property real panelWidthMin: height*0.15
    // width left for the instruments plugins while resizing
    readonly property real instrumentsWidthMin: width*0.1

    function panelWidth(name, defaultWidth)
    {
        var v=groundControl.panelSize(name)
        return v>0?Math.max(panelWidthMin, control.width*v):defaultWidth
    }

    Component.onCompleted: {
        addComponent(status, "status")
        addComponent(pfd, "pfd")
        addComponent(numbers, "numbers")
        addComponent(commands, "commands")

        instrumentsItem=addComponent(c_instruments)
    }
    property var instrumentsItem

    Component {
        id: status
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Status { } }
            Layout.preferredWidth: control.panelWidth("status", -1)
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
            sourceComponent: Component { NumbersBox { settingsName: "instruments" } }
            Layout.preferredWidth: control.panelWidth("numbers", -1)
        }
    }
    Component {
        id: commands
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Commands { } }
            Layout.preferredWidth: control.panelWidth("commands", -1)
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
                active: groundControl.layoutEditing && sep.name!==""
                property real size0: 0
                property real sizeMax: 0
                onStarted: {
                    size0=sep.panel.width
                    sizeMax=size0+Math.max(0, instrumentsItem.width-instrumentsWidthMin)
                }
                onMoved: (delta) => {
                    var w=apx.limit(size0+delta, panelWidthMin, sizeMax)
                    groundControl.setPanelSize(sep.name, w/control.width)
                }
                onReset: groundControl.setPanelSize(sep.name, 0)
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
