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

import APX.Facts 1.0

Fact {
    id: setFact
    flags: (Fact.Group | Fact.FlatModel)

    property var values //from config

    signal selected(var num)

    property int editorsCnt: 1
    Fact {
        id: setTitle
        title: qsTr("Description")
        flags: Fact.Text
        icon: "rename-box"
        value: setFact.title
        onValueChanged: {
            setFact.title=value
        }
    }

    NumbersMenuNumber
    {
        title: qsTr("Add new item")
        icon: "plus-circle"
        newItem: true
        onAddTriggered: createNumber(save())
    }

    Fact {
        id: setValues
        title: qsTr("Values")
        flags: (Fact.Group | Fact.Section | Fact.DragChildren)
    }

    Component.onCompleted: updateSetItems()

    function save()
    {
        var values=[]
        for(var i=0;i<setValues.size;++i){
            var number=setValues.child(i).save()
            if(!number.bind) continue
            values.push(number)
        }
        if(!values)return
        var set={}
        set.title=title
        set.values=values
        return set
    }

    function updateSetItems()
    {
        setValues.onSizeChanged.disconnect(updateDescr)
        setValues.deleteChildren();
        for(var i in values){
            createNumber(values[i])
        }
        updateDescr()
        setValues.onSizeChanged.connect(updateDescr)
    }
    function createNumber(number)
    {
        if(!number.bind)return
        if(number.bind==="")return
        var c=createFact(setValues, "NumbersMenuNumber.qml", {"data": number})
        c.titleChanged.connect(updateDescr)
        c.removeTriggered.connect(updateDescr)
    }
    function updateDescr()
    {
        if(!setFact) return
        descr=""
        var s=[]
        for(var i=0;i<setValues.size;++i){
            s.push(setValues.child(i).title)
        }
        descr=s.join(',')
    }

    Fact {
        flags: (Fact.Action | Fact.Remove)
        title: qsTr("Remove set")
        icon: "delete"
        onTriggered: {
            if(setFact.active)select(0)
            setFact.destroy()
        }
    }
    Fact {
        flags: (Fact.Action | Fact.Apply)
        title: qsTr("Select and save")
        visible: !setFact.active
        icon: "check-circle"
        onTriggered: {
            setFact.menuBack()
            setFact.selected(setFact.num)
        }
    }
}
