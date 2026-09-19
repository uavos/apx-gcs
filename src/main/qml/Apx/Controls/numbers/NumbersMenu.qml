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
    id: setsFact
    property var defaults
    property string settingsName
    property bool destroyOnClose: true
    property var loadedFiles: []

    name: settingsName
    flags: (Fact.Group | Fact.DragChildren)
    title: qsTr("Numbers")+": "+settingsName
    descr: qsTr("Instrument editor")
    icon: "gauge"

    signal accepted()

    Component.onCompleted: open()

    function open() {
        //ensure mandala linked to unit
        if(!parentFact){
            var p=parent
            parentFact=apx.fleet.local
            parent=p
        }
        loadSettings()
    }

    function close()
    {
        if(!destroyOnClose){
            setsFact.deleteChildren()
            loadSettings()
            menuBack()
            return
        }
        setsFact.deleteChildren()
        menuBack()
        parentFact=null
    }

    function loadSettings()
    {
        var sets=[]
        var set={}
        var currentSetIdx=-1
        var activeFile=application.prefs.loadValue(settingsName, "numbers/active", "")
        var fileNames=application.prefs.files("numbers-*.json")
        loadedFiles=[]
        for(var i=0;i<fileNames.length;++i){
            var fileName=fileNames[i]
            var f=application.prefs.loadFile(fileName)
            if(!f) continue
            try {
                set=JSON.parse(f)
            } catch(e) {
                console.warn("Can't parse "+fileName+": "+e)
                continue
            }
            if(!(set.values && (set.values instanceof Array))) continue
            set.sourceFile=fileName
            sets.push(set)
            loadedFiles.push(fileName)
            if(fileName===activeFile)
                currentSetIdx=sets.length-1
        }

        // Import the old combined format. It will be split on the next save.
        if(sets.length===0){
            var legacyFile=application.prefs.loadFile("numbers.json")
            var legacy={}
            try {
                legacy=legacyFile?JSON.parse(legacyFile):{}
            } catch(e) {
                console.warn("Can't parse numbers.json: "+e)
            }
            if(legacy && legacy.sets){
                for(i in legacy.sets){
                    set=legacy.sets[i]
                    if(!(set.values && (set.values instanceof Array))) continue
                    sets.push(set)
                }
                if(legacy.active){
                    var setIdx=legacy.active[settingsName]
                    if(setIdx>=0 && setIdx<sets.length)
                        currentSetIdx=setIdx
                }
            }
        }
        //defaults
        if(sets.length<=0){
            set={}
            set.title=settingsName
            set.values=defaults
            sets.push(set)
            currentSetIdx=sets.length-1
        }
        else if(currentSetIdx<0)
            currentSetIdx=0

        //create facts
        for(i in sets){
            var opts=sets[i]
            var c=createFact(setsFact, "NumbersMenuSet.qml", opts)
            c.selected.connect(select)
            c.selected.connect(saveSettings)
        }
        select(currentSetIdx)
    }

    function saveSettings()
    {
        var usedNames={}
        var savedFiles=[]
        var activeFile=""
        for(var i=0;i<size;++i){
            var setFact=child(i)
            var set=setFact.save()
            if(!set)continue
            var title=uniqueTitle(set.title, usedNames)
            set.title=title
            setFact.title=title
            var fileName="numbers-"+fileSafeName(title)+".json"
            application.prefs.saveFile(fileName,JSON.stringify(set,' ',2))
            savedFiles.push(fileName)
            if(setFact.active)
                activeFile=fileName
        }
        for(i=0;i<loadedFiles.length;++i){
            if(savedFiles.indexOf(loadedFiles[i])<0)
                application.prefs.removeFile(loadedFiles[i])
        }
        if(savedFiles.length>0)
            application.prefs.removeFile("numbers.json")
        application.prefs.saveValue(settingsName, activeFile, "numbers/active")
        accepted()
        close()
    }

    function fileSafeName(title)
    {
        var name=title.trim().replace(/[\\\/:*?"<>|]/g, "-")
        name=name.replace(/^\.+|\.+$/g, "")
        return name || "set"
    }

    function uniqueTitle(title, usedNames)
    {
        var base=title.trim() || "set"
        var name=base
        var copy=0
        while(usedNames[fileSafeName(name).toLowerCase()]){
            name=base+"-copy"+(copy>0?"("+copy+")":"")
            ++copy
        }
        usedNames[fileSafeName(name).toLowerCase()]=true
        return name
    }

    function createFact(parent, url, opts)
    {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c=component.createObject(parent,opts)
            c.parentFact=parent
            return c
        }
    }

    function select(num)
    {
        for(var i=0;i<setsFact.size;++i){
            var set=setsFact.child(i)
            set.active = set.num == num
        }
    }

    Fact {
        title: qsTr("Add set")
        flags: Fact.Action
        icon: "plus-circle"
        onTriggered: {
            var set={}
            set.title="#"+(setsFact.size+1)
            set.values=[]
            var c=createFact(setsFact, "NumbersMenuSet.qml", set)
            c.selected.connect(select)
            c.selected.connect(saveSettings)
            c.trigger()
        }
    }

    Fact {
        title: qsTr("Save")
        flags: (Fact.Action | Fact.Apply)
        icon: "check-circle"
        onTriggered: saveSettings()
    }
}
