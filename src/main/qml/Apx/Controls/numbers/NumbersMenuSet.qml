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
        setValues.removeAll();
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
