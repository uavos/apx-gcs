import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Qt.labs.settings 1.0
import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import Apx.Common 1.0
import Apx.Menu 1.0
import APX.Facts 1.0

Dialog {
    id: editor

    property var defaults
    property string settingsName

    visible: true
    title: qsTr("Numbers display editor")
    standardButtons: Dialog.Ok | Dialog.Cancel
    onClosed: destroy()
    modal: false
    closePolicy: Popup.CloseOnEscape

    x: (parent.width-width)/2
    y: (parent.height-height)/2

    implicitHeight: Math.max(implicitWidth*1.2,contentHeight+header.height+footer.height)

    padding: 0


    onAccepted: saveSettings()

    property color headerBG: Material.color(Material.BlueGrey)
    property color cellBG: Material.color(Material.Grey)
    contentItem: FactMenu {
        id: factMenu
        fact: menu
    }

    property int currentSetIdx: -1

    property var sets: {
        var v=[]
        var f=apx.settings.loadFile("numbers.json")
        var json=f?JSON.parse(f):{}
        var set={}
        if(json && json.sets){
            for(var i in json.sets){
                set=json.sets[i]
                if(!(set.values && (set.values instanceof Array))) continue
                v.push(set)
            }
            //set index
            var setIdx=json.active[settingsName]
            if(setIdx>=0 && setIdx<v.length)currentSetIdx=setIdx
            else if(v.length>0)currentSetIdx=0
        }
        //defaults
        if(v.length<=0 || !json.active){
            set={}
            set.title=settingsName
            set.values=defaults
            v.push(set)
            currentSetIdx=v.length-1
        }
        return v
    }

    function saveSettings()
    {
        var f=apx.settings.loadFile("numbers.json")
        var json=f?JSON.parse(f):{}
        if(!json.active)json.active={}
        json.active[settingsName]=currentSetIdx
        json.sets=[]
        for(var i in sets){
            var set=sets[i]
            var values=set.values
            if(!values)continue
            var o={}
            o.title=set.title
            o.values=values
            json.sets.push(o)
        }
        apx.settings.saveFile("numbers.json",JSON.stringify(json,' ',2))
    }



    Component.onDestruction: menu.remove()
    Fact {
        id: menu
        name: settingsName
        treeType: Fact.Group
        Component.onCompleted: {
            //ensure mandala linked to vehicle
            parentFact=apx.vehicles.current
            //parent=editor

            updateItems()
        }

        function updateItems()
        {
            removeAll()
            for(var i=0;i<sets.length;++i){
                var c=menuSetC.createObject(menu,{"setIdx": i})
                c.parentFact=menu
            }
        }
        Fact {
            title: qsTr("Add set")
            treeType: Fact.Action
            dataType: Fact.Apply
            icon: "plus-circle"
            onTriggered: {
                var obj={}
                obj.title="#"+(sets.length+1)
                obj.values=[]
                sets.unshift(obj)
                menu.updateItems()
                factMenu.openFact(menu.child(0))
            }
        }
    }
    Component {
        id: menuSetC
        Fact {
            id: menuSet
            property int setIdx
            title: sets[setIdx].title
            treeType: Fact.Group
            active: setIdx==currentSetIdx
            property int editorsCnt: 1
            Fact {
                id: setTitle
                title: qsTr("Description")
                dataType: Fact.Text
                icon: "rename-box"
                value: sets[setIdx].title
                onValueChanged: {
                    if(setIdx<sets.length){
                        sets[setIdx].title=value
                        menuSet.title=value
                    }
                }
            }
            property var setFacts: ([])

            Component.onCompleted: updateSetItems()
            function updateSetItems()
            {
                onSizeChanged.disconnect(updateDescr)
                for(var i in setFacts){
                    setFacts[i].remove()
                }
                setFacts=[]
                var c=menuNumberC.createObject(menuSet,{"setIdx": setIdx}) //add number menu
                c.addTriggered.connect(function(){addNewItem(sets[setIdx].values.length-1)})
                c.parentFact=menuSet
                setFacts.push(c)

                for(i in sets[setIdx].values){
                    addNewItem(i)
                }
                updateDescr()
                onSizeChanged.connect(updateDescr)
            }
            function addNewItem(i)
            {
                var list=sets[setIdx].values
                if(!list[i].bind)return
                if(list[i].bind==="")return
                var c=menuNumberC.createObject(menuSet,{"setIdx": setIdx, "row": i})
                c.titleChanged.connect(updateDescr)
                c.removeTriggered.connect(updateDescr)
                c.parentFact=menuSet
                setFacts.push(c)
            }
            function updateDescr()
            {
                if(!menuSet)return
                menuSet.descr=""
                var s=[]
                for(var i in setFacts){
                    s.push(setFacts[i].title)
                }
                menuSet.descr=s.join(',')
            }

            Fact {
                treeType: Fact.Action
                dataType: Fact.Apply
                title: qsTr("Select")
                icon: "check-circle"
                visible: !menuSet.active
                onTriggered: {
                    factMenu.back()
                    currentSetIdx=menuSet.setIdx
                }
            }
            Fact {
                treeType: Fact.Action
                dataType: Fact.Remove
                title: qsTr("Remove set")+" ("+menuSet.title+")"
                icon: "delete"
                onTriggered: {
                    sets.splice(setIdx,1)
                    menuSet.destroy()
                    menu.updateItems()
                }
            }
        }
    }

    Component {
        id: menuNumberC
        Fact {
            id: menuNumber
            treeType: Fact.Group

            property int setIdx
            property int row: -1

            signal addTriggered()
            signal removeTriggered()

            property var list: sets[setIdx].values
            property var obj: (row>=0 && row<list.length)?list[row]:{}
            icon: row<0?"plus-circle":""
            section: row<0?"":qsTr("Values")


            onTriggered: if(row<0)obj={}

            function updateDescr()
            {
                var s=row<0?qsTr("Add new item"):obj.title?obj.title:obj.bind
                title=s?s:""

                var descrList=[]
                for(var p in obj){
                    if(p==="title")continue
                    if(obj[p]==="")continue
                    descrList.push(p.toUpperCase()+": "+obj[p])
                }
                if(descrList.length>0)descr=descrList.join(", ")
                else descr=""
            }
            Component.onCompleted: updateDescr()
            Fact {
                id: mFact
                title: qsTr("Binding")
                descr: qsTr("Fact value")
                dataType: Fact.Mandala
                onValueChanged: {
                    //console.log(text)
                    mBind.setValue(text)
                    value=""
                }
            }
            Fact {
                id: mBind
                title: qsTr("Expression")
                descr: "Math.atan(m.pitch.value/m.roll.value)"
                dataType: Fact.Text
                value: obj.bind?obj.bind:""
                onValueChanged: {
                    var s=value.trim()
                    if(s!==value){
                        setValue(s)
                        return
                    }
                    if(value){
                        obj.bind=value
                        updateDescr()
                    }else if(obj.bind){
                        setValue(obj.bind)
                    }
                }
            }
            Fact {
                title: qsTr("Title")
                descr: qsTr("Label text")
                dataType: Fact.Text
                value: obj.title?obj.title:""
                onValueChanged: {
                    obj.title=value
                    updateDescr()
                }
            }
            Fact {
                title: qsTr("Precision")
                descr: qsTr("Digits after decimal point")
                dataType: Fact.Text
                value: obj.prec?obj.prec:""
                onValueChanged: {
                    obj.prec=value
                    updateDescr()
                }
            }
            Fact {
                title: qsTr("Warning")
                descr: qsTr("Expression for warning")
                dataType: Fact.Text
                value: obj.warn?obj.warn:""
                onValueChanged: {
                    obj.warn=value
                    updateDescr()
                }
            }
            Fact {
                title: qsTr("Alarm")
                descr: "value>1.8 || (value>0 && value<1)"
                dataType: Fact.Text
                value: obj.alarm?obj.alarm:""
                onValueChanged: {
                    obj.alarm=value
                    updateDescr()
                }
            }
            Fact {
                title: qsTr("Action")
                descr: "m.stage.value=100"
                dataType: Fact.Text
                value: obj.act?obj.act:""
                onValueChanged: {
                    obj.act=value
                    updateDescr()
                }
            }
            //actions
            Fact {
                treeType: Fact.Action
                dataType: Fact.Apply
                title: qsTr("Add")
                enabled: (row<0 && mBind && mBind.value)?true:false
                icon: "plus-circle"
                onTriggered: {
                    list.push(obj)
                    factMenu.back()
                    addTriggered()
                }
            }
            Fact {
                treeType: Fact.Action
                dataType: Fact.Remove
                title: qsTr("Remove")
                visible: row>=0
                icon: "delete"
                onTriggered: {
                    list.splice(row,1)
                    menuNumber.destroy()
                    removeTriggered()
                }
            }
        }
    }
}
