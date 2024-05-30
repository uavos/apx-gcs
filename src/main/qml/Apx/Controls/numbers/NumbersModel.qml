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

import QtQml.Models
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Menu
import Apx.Common

import APX.Facts

ObjectModel {
    id: model

    property real minimumWidth: 0
    property real itemHeight: Style.buttonSize

    property bool light: false
    property bool fixedWidth: false

    property string settingsName: "default"


    property var defaults: [
    ]

    function loadSettings()
    {
        var f=application.prefs.loadFile("numbers.json")
        var json=f?JSON.parse(f):{}
        var list=defaults
        while(json){
            var set
            if(json.sets && json.active)
                set=json.sets[json.active[settingsName]]
            if(!set) break
            var values=set["values"]
            if(!values) break
            if(!(values instanceof Array))break;
            list=values
            break;
        }
        updateNumbers(list)
    }

    Component.onCompleted: {
        loadSettings()
    }

    property var objList: [ ]

    function updateNumbers(list)
    {
        clearObjList()
        model.clear()
        model.minimumWidth=itemHeight*3
        for(var i in list){
            var n=list[i]
            //console.log(n.bind)
            var s="import QtQuick 2.0; NumbersItem {"
            s+="light: "+light+";"
            s+="fixedWidth: "+model.fixedWidth+";"
            if(n.warn)s+="warning: "+n.warn+";"
            if(n.alarm)s+="error: "+n.alarm+";"

            var f=null
            if(!(n.bind.match(/[\(\+!*]/) || n.bind.includes(".value")))
                f=apx.vehicles.current.mandala.fact(n.bind, true)
            if(f){
                s+="fact: mandala."+f.mpath()+";"
                s+="property var v: fact?fact.value:undefined"+";"
            }else{
                s+="property var v: "+n.bind+";"
            }
            if(n.prec){
                s+="value: v.toFixed("+n.prec+")"+";"
            }else if(!f){
                s+="value: v;"
            }
            if(n.act){
                s+="enabled: true;"
                s+="onTriggered: {"+n.act+"}"
            }
            s+="}"
            var obj = Qt.createQmlObject(s,model);

            obj.model=model

            /*if(model.fixedWidth){
                obj.model=model
                obj.minimumWidth=Qt.binding(function(){return model.minimumWidth})
            }else{
                obj.implicitWidth=Qt.binding(function(){return Math.max(obj.defaultWidth, obj.width)})
            }*/

            obj.height=Qt.binding(function(){return itemHeight})
            for(var p in n){
                if(typeof(obj[p])=='undefined')continue
                if(n[p]==="")continue
                obj[p]=n[p]
            }
            model.append(obj)
            objList.push(obj)
        }
    }

    function clearObjList()
    {
        for(var i in objList){
            objList[i].destroy()
            delete objList[i]
        }
    }

    //EDITOR
    property Popup popup
    function edit()
    {
        if(popup)return

        var c=Qt.createComponent("NumbersMenuPopup.qml",Component.PreferSynchronous,ui.window)
        if (c.status === Component.Ready) {
            var obj = c.createObject(ui.window,{
                                         "defaults": model.defaults,
                                         "settingsName": model.settingsName
                                     })
            popup=obj
            popup.accepted.connect(loadSettings)
            popup.open()
        }else{
            console.log(c.errorString())
        }
    }

}
