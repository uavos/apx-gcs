import QtQuick 2.12

import APX.Facts 1.0

Fact {
    id: setsFact
    property var defaults
    property string settingsName
    property bool destroyOnClose: true

    name: settingsName
    flags: (Fact.Group | Fact.DragChildren)
    title: qsTr("Numbers")+": "+settingsName
    descr: qsTr("Instrument editor")
    icon: "gauge"

    signal accepted()

    Component.onCompleted: open()

    function open() {
        //ensure mandala linked to vehicle
        if(!parentFact){
            var p=parent
            parentFact=apx.vehicles.local
            parent=p
        }
        loadSettings()
    }

    function close()
    {
        if(!destroyOnClose){
            setsFact.removeAll()
            loadSettings()
            menuBack()
            return
        }
        setsFact.removeAll()
        menuBack()
        parentFact=null
    }

    function loadSettings()
    {
        var sets=[]
        var f=application.prefs.loadFile("numbers.json")
        var json=f?JSON.parse(f):{}
        var set={}
        var currentSetIdx=-1
        if(json && json.sets){
            for(var i in json.sets){
                set=json.sets[i]
                if(!(set.values && (set.values instanceof Array))) continue
                sets.push(set)
            }
            //set index
            var setIdx=json.active[settingsName]
            if(setIdx>=0 && setIdx<sets.length)
                currentSetIdx=setIdx
            else if(sets.length>0)
                currentSetIdx=0
        }
        //defaults
        if(sets.length<=0 || !json.active){
            set={}
            set.title=settingsName
            set.values=defaults
            sets.push(set)
            currentSetIdx=sets.length-1
        }

        //create facts
        for(i in sets){
            var c=createFact(setsFact, "NumbersMenuSet.qml", sets[i])
            c.selected.connect(select)
            c.selected.connect(saveSettings)
        }
        select(currentSetIdx)
    }

    function saveSettings()
    {
        var fjson=application.prefs.loadFile("numbers.json")
        var json=fjson?JSON.parse(fjson):{}
        if(!json.active)json.active={}
        json.active[settingsName]=0
        json.sets=[]
        for(var i=0;i<size;++i){
            var setFact=child(i)
            var set=setFact.save()
            if(!set)continue
            json.sets.push(set)
            if(setFact.active)
                json.active[settingsName]=i
        }
        application.prefs.saveFile("numbers.json",JSON.stringify(json,' ',2))
        accepted()
        close()
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
