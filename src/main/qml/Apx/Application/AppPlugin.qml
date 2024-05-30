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
import QtQml

Loader {
    id: plugin
    property string uiComponent
    signal configure()  // called when uiComponent is loaded

    property bool unloadOnHide: true

    property var fact   //set by C++ Fact->loadQml()
    property string name: fact?fact.name:""
    property string title: fact?fact.title:""
    property string descr: fact?fact.descr:""
    property string icon: fact?fact.icon:""

    //forward reference to adjust view states of widget, i.e. "maximized", "minimized"
    readonly property string pluginState: state
    readonly property bool pluginMinimized: pluginState=="minimized"


    visible: true

    active: false
    asynchronous: true
    Component.onCompleted: {
        if(uiComponent && (!(ui && ui[uiComponent])))return
        activate()
    }

    function activate(object)
    {
        active=unloadOnHide?Qt.binding(function(){return visible}):true
        configure()
    }
    Connections {
        target: application
        enabled: uiComponent
        function onUiComponentLoaded(name,object){
            if(name===plugin.uiComponent)
                activate(object)
        }
    }
}
