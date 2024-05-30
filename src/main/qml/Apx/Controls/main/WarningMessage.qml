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
import QtQuick.Layouts

import Apx.Common
//import Apx.Menu 1.0
import APX.Vehicles

FactButton {
    id: control

    showText: enabled
    showValue: false
    showNext: false
    showEditor: false

    enabled: fact.size

    fact: apx.vehicles.current.warnings
    readonly property int showTimeout: 5000
    readonly property int showTimes: 3

    Connections {
        target: fact
        function onShow(msg, msgType){ message(msg, msgType) }
    }

    state: "NORMAL"
    states: [
        State {
            name: "NORMAL"
            PropertyChanges {
                target: control
                highlighted: false
                iconName: "alert-circle-outline"
                text: fact.text
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
                text: showItem.title
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
        var i=msg.lastIndexOf(":")
        if(i>0){
            sdescr=msg.slice(i+1).trim()
            stitle=msg.slice(0,i).trim()
        }
        stitle = apx.vehicles.current.title+": "+stitle
        model.add(msg, {"title": stitle, "descr": sdescr, "icon": sicon})
    }

}
