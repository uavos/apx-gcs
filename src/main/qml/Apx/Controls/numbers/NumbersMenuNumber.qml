import QtQuick 2.12

import APX.Facts 1.0

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
            var v=data[f.name]
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
            data[f.name]=s
        }
        return data
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
        flags: Fact.Mandala
        onValueChanged: {
            var v=text?text:status
            mBind.setValue(v)
            value=""
            status=""
        }
    }
    Fact {
        id: mBind
        name: "bind"
        title: qsTr("Expression")
        descr: "Math.atan(m.pitch.value/m.roll.value)"
        flags: Fact.Text
        onValueChanged: updateDescr()
    }
    Fact {
        id: mTitle
        name: "title"
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
        descr: "m.stage.value=100"
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
            number.remove()
        }
    }
}
