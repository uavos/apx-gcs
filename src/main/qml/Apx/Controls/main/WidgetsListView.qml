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
import QtQuick.Controls
import QtCore

import Apx.Common
import Apx.Controls


ListView {
    id: control
    implicitWidth: contentWidth
    implicitHeight: contentItem.childrenRect.height
    spacing: Style.spacing
    model: pluginsModel
    visible: count>0
    orientation: ListView.Horizontal

    delegate: IconButton {
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
        for(var i in plugin) {
            var v=plugin[i]
            if(v) p[i]=v
        }
        p.idx = plugins.length-1
        pluginsModel.insert(index, p)
        plugin.active=false
    }
}
