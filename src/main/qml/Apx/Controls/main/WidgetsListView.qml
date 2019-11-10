import QtQuick 2.12
import QtQuick.Controls 2.12

import Apx.Common 1.0
import Apx.Controls 1.0

import Qt.labs.settings 1.0


ListView {
    id: control
    implicitWidth: contentWidth
    implicitHeight: contentItem.childrenRect.height
    spacing: 3
    model: pluginsModel
    visible: count>0
    orientation: ListView.Horizontal

    delegate: CleanButton {
        property var plugin: plugins[model.idx]
        text: plugin.title
        toolTip: plugin.descr
        iconName: plugin.icon
        highlighted: plugin.active
        onTriggered: {
            plugin.active=!plugin.active
            settings.setValue(plugin.title,plugin.visible)
        }
    }

    ListModel {
        id: pluginsModel
    }
    property var plugins: []

    Settings {
        id: settings
        category: "widgets"
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
        plugin.active=false //settings.value(plugin.title,false)===true
    }
}
