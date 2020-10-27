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
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Qt.labs.settings 1.0
import QtQml.Models 2.13

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0
import Apx.Application 1.0

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

    function addComponent(c)
    {
        var obj=c.createObject(control,{"Layout.fillHeight": true})
        c_vsep.createObject(control)
        return obj
    }

    Component.onCompleted: {
        addComponent(status)
        addComponent(pfd)
        addComponent(numbers)
        addComponent(commands)

        instrumentsItem=addComponent(c_instruments)
    }
    property var instrumentsItem

    Component {
        id: status
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Status { } }
        }
    }
    Component {
        id: pfd
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Pfd { } }
            Layout.preferredWidth: control.height*2
        }
    }
    Component {
        id: numbers
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { NumbersBox { settingsName: "instruments" } }
        }
    }
    Component {
        id: commands
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Commands { } }
        }
    }

    Component {
        id: c_vsep
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 1
            border.width: 0
            color: sepColor
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
                Layout.margins: 3
                implicitWidth: contentWidth
                spacing: 3
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
