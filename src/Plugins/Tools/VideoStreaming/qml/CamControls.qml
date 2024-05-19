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
import QtQuick.Controls
import QtQuick.Layouts

import Apx.Common
import Apx.Controls

Item {
    id: control

    property int size: 100
    property int w: 32
    property int ctrSize: Math.min(control.width,control.height)/1.5

    property var f_cmdX: mandala.findChild(plugin.tune.controls.control_x.text)
    property var f_cmdY: mandala.findChild(plugin.tune.controls.control_y.text)
    property var f_zoom: mandala.cmd.cam.zoom

    property real spanX: plugin.tune.controls.control_sx.value
    property real spanY: plugin.tune.controls.control_sy.value*(rev_y?1:-1)

    property bool rev_zoom: plugin.tune.controls.rev_zoom.value
    property bool rev_y: plugin.tune.controls.rev_y.value

    property bool dragEnabled: (f_cmdX?true:false)||(f_cmdY?true:false)

    function setValue(fact, v=0, span=0)
    {
        if(!fact)return
        if(v>1)v=1
        else if(v<-1)v=-1
        fact.value=v*span
    }

    clip: true

    Rectangle {
        id: rect
        anchors.centerIn: parent
        height: control.size
        width: height
        border.width: 1
        border.color: alive?"#0f0":"#888"
        color: "#00000000"
    }

    Rectangle {
        id: rectActive
        anchors.centerIn: rect
        height: control.ctrSize
        width: height
        border.width: 1
        border.color: "#a0ffffff"
        color: "#00000000"
        visible: mouseArea.dragging || pinchArea.active
    }

    Rectangle {
        id: rectPos
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: cX
        anchors.verticalCenterOffset: cY
        readonly property real cX: f_cmdX?f_cmdX.value*(rectActive.width/2)/spanX:0
        readonly property real cY: f_cmdY?f_cmdY.value*(rectActive.height/2)/spanY:0
        height: control.w
        width: height
        radius: height/2
        border.width: 1
        border.color: "#aaa"
        color: "#222"
        visible: cX!=0 || cY!=0
    }
    MouseArea {
        id: mouseArea
        property bool dragging: false
        property point p
        property real pTime
        anchors.fill: rect
        //hoverEnabled: true
        onPressed: {
            //console.log("PRESS")
            pTime=new Date().getTime()
            p=Qt.point(mouse.x,mouse.y)
        }
        onReleased: {
            //console.log("RELEASE")
            dragging=false
            setValue(f_cmdX)
            setValue(f_cmdY)
        }
        onMouseXChanged: {
            if(!dragEnabled)return
            if(!pressed)return
            if(!dragging) dragging=mouseX!==p.x
            setValue(f_cmdX, (mouseX-p.x)/(rectActive.width/2), spanX)
        }
        onMouseYChanged: {
            if(!dragEnabled)return
            if(!pressed)return
            if(!dragging) dragging=mouseY!==p.y
            setValue(f_cmdY, (mouseY-p.y)/(rectActive.height/2), spanY)
        }
        onWheel: {
            if(!f_zoom) return
            var d=wheel.angleDelta.y
            var v=Math.max(0.001,Math.abs(d*0.01/100))
            if(rev_zoom) d=-d
            if(d<0) zoomSet=Math.max(0,zoomSet-v)
            else if(d>0) zoomSet=Math.min(1,zoomSet+v)
            zoomTimer.start()
        }
        /*onClicked: {
            var dt=new Date().getTime()-pTime
            if(dt>200)return
            //console.log("CLICK"+dt)
        }*/
    }
    PinchArea {
        id: pinchArea
        anchors.fill: parent
        readonly property bool active: pinch.active
        pinch.target: rectActive
        enabled: f_zoom
        pinch.minimumScale: 0.1
        pinch.maximumScale: 10
        property real zoom: 0
        onPinchStarted: zoom=f_zoom.value
        onPinchFinished: rectActive.scale=1
        onPinchUpdated: {
            var v=pinch.scale
            if(v===1) return
            if(v<1) zoomSet=Math.max(0,zoom-1/v/10+0.1)
            else zoomSet=Math.min(1,zoom+(v/10-0.1))
            zoomTimer.start()
        }
    }

    property real zoomSet: 0
    Timer {
        id: zoomTimer
        interval: 200
        onTriggered: {
            zoomSet=(zoomSet*100).toFixed()/100
            f_zoom.value=zoomSet
        }

    }

}
