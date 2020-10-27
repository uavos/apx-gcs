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
import QtQuick 2.12
import QtQuick.Controls 2.12


ListView {
    id: listView

    clip: true

    signal pid(var text, var color)

    ListModel {
        id: _model
    }
    Connections {
        target: apx.protocols.trace
        function onPacket(uplink, packet) {
            if(_model.count>1000)
                _model.remove(0)
            for(var i in packet){
                if(filters.indexOf(packet[i].text)>-1){
                    pid(packet[i].text,"#fff")
                    return
                }
            }
            var dict = {}
            dict.uplink = uplink
            dict.packet = packet
            _model.append(dict)
        }
    }

    property var filters: []
    function filter(text, v)
    {
        if(filters.indexOf(text)>-1){
            if(!v) filters.splice(filters.indexOf(text))
        }else if(v) filters.push(text)
    }


    spacing: 2

    model: _model
    delegate: DatalinkInspectorPacket {
        width: listView.width
        uplink: model.uplink
        packet: model.packet
        onPid: listView.pid(text,color)
    }

    add: Transition {
        id: transAdd
        enabled: ui.smooth
        NumberAnimation {
            properties: "x";
            from: listView.width * (transAdd.ViewTransition.item.uplink?-1:1);
            duration: 100
            easing.type: Easing.OutCubic
        }
    }


    ScrollBar.vertical: ScrollBar {
        width: 6
        active: !listView.atYEnd
    }
    boundsBehavior: Flickable.StopAtBounds
    readonly property bool scrolling: dragging||flicking
    property bool stickEnd: false
    onAtYEndChanged: {
        if(atYEnd)stickEnd=scrolling
        else if(stickEnd)scrollToEnd()
        else if(!(scrolling||atYEnd||scrollTimer.running)) scrollToEnd()//scrollTimerEnd.start()
    }
    onScrollingChanged: {
        //console.log(scrolling)
        if(scrolling && (!atYEnd)){
            scrollTimer.stop()
            stickEnd=false
        }//else scrollTimer.restart()
    }
    Timer {
        id: scrollTimer
        interval: 2000
        onTriggered: scrollTimerEnd.start()
    }
    Timer {
        id: scrollTimerEnd
        interval: 1
        onTriggered: if(!listView.scrolling)listView.scrollToEnd()
    }

    Component.onCompleted: scrollToEnd()

    focus: false
    keyNavigationEnabled: false

    function scrollToEnd()
    {
        positionViewAtEnd()
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            //listView.footerItem.focusRequested()
            listView.scrollToEnd()
        }
    }
}
