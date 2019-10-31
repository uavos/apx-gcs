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

                plugins.push(plugin)
                var p = {}
                for(var i in plugin) p[i]=plugin[i]
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
                delegate: CleanButton {
                    property var plugin: plugins[model.idx]
                    text: plugin.title
                    toolTip: plugin.descr
                    iconName: plugin.icon
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
