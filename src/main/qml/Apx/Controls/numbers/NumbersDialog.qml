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



    FactObject {
        id: menu
        Component.onCompleted: updateItems()
        function updateItems()
        {
            clearChildren()
            for(var i=0;i<sets.length;++i){
                add(menuSetC.createObject(this,{"setIdx": i}))
            }
        }
        actions: [
            FactObject {
                title: qsTr("Add set")
                flags: FactAction.ActionApply
                icon: "plus-circle"
                onTriggered: {
                    var obj={}
                    obj.title="#"+(sets.length+1)
                    obj.values=[]
                    sets.unshift(obj)
                    menu.updateItems()
                    factMenu.openFact(menu.children[0])
                }
            }
        ]
    }
    Component {
        id: menuSetC
        FactObject {
            id: menuSet
            property int setIdx
            title: sets[setIdx].title
            treeType: Fact.Group
            active: setIdx==currentSetIdx
            property int editorsCnt: 1
            FactObject {
                id: setTitle
                title: qsTr("Description")
                dataType: Fact.Text
                icon: "rename-box"
                m_value: sets[setIdx].title
                onValueUpdated: {
                    if(setIdx<sets.length){
                        sets[setIdx].title=v
                        menuSet.title=v
                    }
                }
            }

            Component.onCompleted: updateSetItems()
            function updateSetItems()
            {
                onChildrenChanged.disconnect(updateDescr)
                clearChildren(editorsCnt)
                var list=sets[setIdx].values
                var c=menuNumberC.createObject(this,{"setIdx": setIdx}) //add number menu
                c.addTriggered.connect(updateSetItems)
                add(c)
                for(var i=0;i<list.length;++i){
                    if(!list[i].bind)continue
                    if(list[i].bind==="")continue
                    c=menuNumberC.createObject(this,{"setIdx": setIdx, "row": i})
                    c.titleChanged.connect(updateDescr)
                    c.removeTriggered.connect(updateDescr)
                    add(c)
                }
                updateDescr()
                onChildrenChanged.connect(updateDescr)
            }
            function updateDescr()
            {
                if(!menuSet)return
                menuSet.descr=""
                var s=[]
                for(var i in children){
                    if(i<(editorsCnt+1))continue
                    s.push(children[i].title)
                }
                menuSet.descr=s.join(',')
            }

            actions: [
                FactObject {
                    title: qsTr("Select")
                    flags: FactAction.ActionApply
                    icon: "check-circle"
                    visible: !menuSet.active
                    onTriggered: {
                        factMenu.back()
                        currentSetIdx=menuSet.setIdx
                    }
                },
                FactObject {
                    title: qsTr("Remove set")+" ("+menuSet.title+")"
                    flags: FactAction.ActionRemove
                    icon: "delete"
                    onTriggered: {
                        sets.splice(setIdx,1)
                        menuSet.destroy()
                        menu.updateItems()
                    }
                }
            ]
        }
    }

    Component {
        id: menuNumberC
        FactObject {
            id: menuNumber
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
            FactObject {
                id: mFact
                title: qsTr("Binding")
                descr: qsTr("Fact value")
                dataType: Fact.Mandala
                //m_value: obj?obj.bind:""
                onValueUpdated: {
                    mBind.setValue(value)
                    m_value=""
                }
            }
            FactObject {
                id: mBind
                title: qsTr("Expression")
                descr: "Math.atan(m.pitch.value/m.roll.value)"
                dataType: Fact.Text
                m_value: obj.bind?obj.bind:""
                onValueUpdated: {
                    var s=v.trim()
                    if(s!==v){
                        setValue(s)
                        return
                    }
                    if(v){
                        obj.bind=value
                        updateDescr()
                    }else if(obj.bind){
                        setValue(obj.bind)
                    }
                }
            }
            FactObject {
                title: qsTr("Title")
                descr: qsTr("Label text")
                dataType: Fact.Text
                m_value: obj.title?obj.title:""
                onValueUpdated: {
                    obj.title=value
                    updateDescr()
                }
            }
            FactObject {
                title: qsTr("Precision")
                descr: qsTr("Digits after decimal point")
                dataType: Fact.Text
                m_value: obj.prec?obj.prec:""
                onValueUpdated: {
                    obj.prec=value
                    updateDescr()
                }
            }
            FactObject {
                title: qsTr("Warning")
                descr: qsTr("Expression for warning")
                dataType: Fact.Text
                m_value: obj.warn?obj.warn:""
                onValueUpdated: {
                    obj.warn=value
                    updateDescr()
                }
            }
            FactObject {
                title: qsTr("Alarm")
                descr: "value>1.8 || (value>0 && value<1)"
                dataType: Fact.Text
                m_value: obj.alarm?obj.alarm:""
                onValueUpdated: {
                    obj.alarm=value
                    updateDescr()
                }
            }
            FactObject {
                title: qsTr("Action")
                descr: "m.stage.value=100"
                dataType: Fact.Text
                m_value: obj.act?obj.act:""
                onValueUpdated: {
                    obj.act=value
                    updateDescr()
                }
            }
            actions: [
                FactObject {
                    title: qsTr("Add")
                    visible: row<0 && mBind.value
                    flags: FactAction.ActionApply
                    icon: "plus-circle"
                    onTriggered: {
                        list.push(obj)
                        factMenu.back()
                        addTriggered()
                    }
                },
                FactObject {
                    title: qsTr("Remove")
                    visible: row>=0
                    flags: FactAction.ActionRemove
                    icon: "delete"
                    onTriggered: {
                        list.splice(row,1)
                        menuNumber.destroy()
                        removeTriggered()
                    }
                }
            ]
        }
    }
}
