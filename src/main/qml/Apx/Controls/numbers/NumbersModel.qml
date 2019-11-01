import QtQuick 2.5;
import QtQuick.Layouts 1.3

import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import Apx.Menu 1.0

import APX.Facts 1.0

ObjectModel {
    id: model

    property int minimumWidth: 0
    property int itemHeight: 32

    property bool light: false
    property bool fixedWidth: false

    property string settingsName: "default"


    property var defaults: [
        //{"bind": "mode"},
        //{"bind": "rc_roll", "title": "RCR", "prec": 0},
        //{"bind": "yaw", "title": "Y", "prec": 1},
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
        model.minimumWidth=0
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

            if(model.fixedWidth){
                obj.model=model
                obj.minimumWidth=Qt.binding(function(){return model.minimumWidth})
            }

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
