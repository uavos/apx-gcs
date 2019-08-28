import QtQuick 2.5;
import QtQuick.Layouts 1.3

import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

ObjectModel {
    id: model

    property int itemWidth: 0
    property int minimumWidth: 0
    property int itemHeight: 32
    property bool light: false

    property string settingsName: "default"


    property var defaults: [
        //{"bind": "mode"},
        //{"bind": "rc_roll", "title": "RCR", "prec": 0},
        //{"bind": "yaw", "title": "Y", "prec": 1},
    ]

    function loadSettings()
    {
        var f=apx.settings.loadFile("numbers.json")
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
        for(var i in list){
            var n=list[i]
            //console.log(n.bind)
            var s="import QtQuick 2.0; NumbersItem {"
            s+="light: "+light+";"
            if(n.warn)s+="warning: "+n.warn+";"
            if(n.alarm)s+="error: "+n.alarm+";"
            if(apx.vehicles.current.mandala.child(n.bind)){
                s+="fact: m."+n.bind+";"
                s+="value: fact?fact.value:''"+";"
            }else{
                s+="value: "+n.bind+";"
            }
            if(n.prec){
                s+="valueText: value.toFixed("+n.prec+")"+";"
            }
            if(n.act){
                s+="enabled: true;"
                s+="onTriggered: {"+n.act+"}"
            }
            s+="}"
            var obj = Qt.createQmlObject(s,model);
            //obj.Layout.fillHeight=true
            if(model.itemWidth)obj.width=Qt.binding(function(){return model.itemWidth})
            if(model.minimumWidth)obj.minimumWidth=Qt.binding(function(){return model.minimumWidth})
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
    function edit()
    {
        var c=Qt.createComponent("NumbersDialog.qml",Component.PreferSynchronous,ui.window)
        if (c.status === Component.Ready) {
            var obj = c.createObject(ui.window,{
                                   "defaults": model.defaults,
                                   "settingsName": model.settingsName
                                   })
            obj.accepted.connect(model.loadSettings)
            obj.open()
        }
    }

}
