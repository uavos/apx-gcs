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

import APX.Facts

Fact {
    id: number
    flags: Fact.Group

    property bool newItem: false

    property var data: ({})

    signal addTriggered()
    signal removeTriggered()

    Component.onCompleted: {
        load(data)
        updateTitle()
        updateDescr()
        mTitle.valueChanged.connect(updateTitle)
    }

    function load()
    {
        for(var i=0;i<size;++i){
            var f=child(i)
            var v=data[settingName(f)]
            f.value=v
        }
    }

    function save()
    {
        data={}
        for(var i=0;i<size;++i){
            var f=child(i)
            var s=f.text.trim()
            if(s === "") continue
            data[settingName(f)]=s
        }
        return data
    }

    function settingName(f)
    {
        var n=f.name
        if(n.includes("_"))
            return n.slice(0,n.indexOf("_"))
        return n
    }

    function updateTitle()
    {
        if(newItem)return
        title=mTitle.text?mTitle.text:mBind.text
    }

    function updateDescr()
    {
        //list non-zero values in descr
        var descrList=[]
        for(var i=0;i<size;++i){
            var f = child(i)
            if(!f.name) continue
            if(f.name === "title") continue
            if(f.text === "") continue
            descrList.push(f.name.toUpperCase()+": "+f.text)
        }
        if(descrList.length>0)descr=descrList.join(", ")
        else descr=""
    }

    Fact {
        id: mFact
        title: qsTr("Binding")
        descr: qsTr("Fact value")
        flags: Fact.Int
        units: "mandala"
        onTextChanged: {
            if(value){
                mBind.setValue(text)
            }
            //value=null
        }
    }
    Fact {
        id: mBind
        name: "bind_num"
        title: qsTr("Expression")
        descr: "Math.atan(est.att.pitch/est.att.roll)"
        flags: Fact.Text
        onValueChanged: updateDescr()
    }
    Fact {
        id: mTitle
        name: "title_num"
        title: qsTr("Title")
        descr: qsTr("Label text")
        flags: Fact.Text
    }
    Fact {
        name: "prec"
        title: qsTr("Precision")
        descr: qsTr("Digits after decimal point")
        flags: Fact.Text
        onValueChanged: updateDescr()
    }
    Fact {
        name: "warn"
        title: qsTr("Warning")
        descr: qsTr("Expression for warning")
        flags: Fact.Text
        onValueChanged: updateDescr()
    }
    Fact {
        name: "alarm"
        title: qsTr("Alarm")
        descr: "value>1.8 || (value>0 && value<1)"
        flags: Fact.Text
        onValueChanged: updateDescr()
    }
    Fact {
        name: "act"
        title: qsTr("Action")
        descr: "cmd.proc.action=proc_action_reset"
        flags: Fact.Text
        onValueChanged: updateDescr()
    }
    //actions
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Add")
        enabled: (newItem && mBind && mBind.value)?true:false
        icon: "plus-circle"
        onTriggered: {
            number.menuBack()
            addTriggered()
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove")
        visible: !newItem
        icon: "delete"
        onTriggered: {
            removeTriggered()
            number.deleteFact()
        }
    }
}
