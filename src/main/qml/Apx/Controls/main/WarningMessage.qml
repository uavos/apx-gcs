import QtQuick 2.13
import QtQuick.Layouts 1.3

import Apx.Common 1.0
//import Apx.Menu 1.0
import APX.Vehicles 1.0

CleanButton {
    id: control
    showText: true

    enabled: fact.size

    readonly property var fact: apx.vehicles.current.warnings
    readonly property int showTimeout: 5000
    readonly property int showTimes: 3

    Connections {
        target: fact
        onShow: message(msg, msgType)
    }
    onTriggered: fact.trigger()

    state: "NORMAL"
    states: [
        State {
            name: "NORMAL"
            PropertyChanges {
                target: control
                highlighted: false
                iconName: "alert-circle-outline"
                title: fact.status
                descr: ""
                toolTip: ""
                opacity: 1
            }
        },
        State {
            name: "NOTIFY"
            PropertyChanges {
                target: control
                highlighted: true
                iconName: showItem.icon
                title: showItem.title
                descr: "["+(model.pos)+"/"+model.count+"] "+showItem.descr
                opacity: 1
            }
        },
        State {
            name: "FALL"
            extend: "NOTIFY"
            PropertyChanges {
                target: control
            }
        }
    ]
    transitions: [
        Transition {
            from: "NOTIFY"
            to: "FALL"
            PropertyAnimation { target: control; property: "opacity"; to: 0; duration: 200}
            onRunningChanged: {
                if(running)return
                state=from
                model.show()
            }
        }
    ]

    property var showItem: ({})

    ListModel {
        id: model
        property int pos: 0

        function show()
        {
            if(count<=0){
                timer.stop()
                control.state="NORMAL"
                pos=0
                return
            }
            if(pos>=count)pos=0
            var item=get(pos)
            if(item.cnt>=showTimes){
                remove(pos)
                show()
                return
            }
            pos++
            showItem.icon=item.icon
            showItem.title=item.title
            showItem.descr=item.descr
            item.cnt++
            if(timer.running)timer.restart()
        }

        function add(msg, item)
        {
            for(var i=0;i<count;++i)
                if(get(i).title===item.title)return
            model.insert(0, {"title": item.title, "descr": item.descr, "icon": item.icon, "cnt": 0, "msg": msg})

            pos=0
            control.state="NORMAL"
            show()
            control.state="NOTIFY"
        }
        property Timer timer: Timer {
            interval: showTimeout
            running: control.state=="NOTIFY"
            onTriggered: control.state="FALL"
        }

    }

    function message(msg, msgType)
    {
        var sicon=""
        switch(msgType){
            case VehicleWarnings.ERROR: sicon="alert-circle"; break;
            case VehicleWarnings.WARNING: sicon="alert-circle-outline"; break;
        }
        var stitle=msg
        var sdescr=""
        var i=msg.indexOf(":")
        if(i>0){
            sdescr=msg.slice(i+1).trim()
            stitle=msg.slice(0,i).trim()
        }
        stitle = apx.vehicles.current.callsign+": "+stitle
        model.add(msg, {"title": stitle, "descr": sdescr, "icon": sicon})
    }

}
