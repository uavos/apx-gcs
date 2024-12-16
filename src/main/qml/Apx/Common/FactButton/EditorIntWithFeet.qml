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
import QtQuick.Controls.Material

import Apx.Common

SpinBox {
    id: editor
    property var opts: fact.opts
    property var measurementsystem: apx.settings.interface.measurementsystem
    property bool tooltip: measurementsystem ? measurementsystem.tooltip.value : false
    property bool isFeets: measurementsystem ? measurementsystem.feets.value : false

    hoverEnabled: true
    
    from: (typeof fact.min!=='undefined')?fact.min*div:-1000000000
    to: (typeof fact.max!=='undefined')?fact.max*div:1000000000
    
    property real div: 1

    value: !isFeets ? fact.value * div : opts.ft * div

    readonly property real precision: fact.precision

    font: apx.font_narrow(factButton.valueSize)

    property int wgrow: implicitWidth
    onImplicitWidthChanged: {
        if(implicitWidth<wgrow)implicitWidth=wgrow
        else wgrow=implicitWidth
    }

    readonly property int defaultStepSize: fact.increment>0?fact.increment*div:1 
    
    stepSize: defaultStepSize * stepMult

    up.onPressedChanged: start()
    down.onPressedChanged: start()

    property int elapsed: 0
    property real startTime: 0
    function start(inc)
    {
        if(activeFocus)
            factButton.forceActiveFocus()

        if(up.pressed || down.pressed)
            startTime = new Date().getTime()
        else{
            startTime = 0
            elapsed = 0
        }
    }
    property int stepMult: 
        elapsed>6
            ? 100
            : elapsed>2
                ? 10
                : 1

    // Temporary meters/feets stub for Runway and Point of interest
    onOptsChanged: hmslCheck()
    function hmslCheck()
    {
        if(fact.name != "hmsl")
            return
        if(fact.parentFact.name == "p#" && opts.ft == 0)  // Point of interest
            opts.ft = "ground"
        if(fact.parentFact.name == "r#" && opts.ft == 0)  // Runway
            opts.ft = "default"
    }
    // Stub END 

    contentItem: Item{
        implicitWidth: isFeets ? textInputFt.width : textInput.width
        
        TextInput {
            id: textInputFt
            visible: isFeets
            validator: IntValidator{bottom: editor.from; top: editor.to}

            anchors.centerIn: parent
            font: editor.font

            color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
            text: opts.ft + " ft"

            activeFocusOnTab: true

            width: Math.max(contentWidth, height*3)

            selectByMouse: true
            onEditingFinished: {
                opts.ft = text
                fact.opts = opts
                fact.setValue(ft2m(text))
                factButton.forceActiveFocus()
                hmslCheck() // Temporary meters/feets stub for Runway and Point of interest
            }
            onActiveFocusChanged: {
                if(activeFocus){
                    text=opts.ft    
                    selectAll();
                }else{
                    text=Qt.binding(function(){return opts.ft + " ft"})
                }
            }
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            Rectangle {
                z: parent.z-1
                visible: fact.enabled
                anchors.centerIn: parent
                width: parent.width+Style.spacing*2
                height: parent.height
                radius: height/10
                color: "#000"
                border.width: 0
                opacity: 0.3
            }
        }
        TextInput {
            id: textInput
            visible: !textInputFt.visible
            property var factValue: fact.value

            anchors.centerIn: parent
            font: editor.font

            color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
            text: fact.text
            activeFocusOnTab: true

            width: Math.max(contentWidth, height*3)

            selectByMouse: true
            onEditingFinished: {
                fact.setValue(text)
                opts.ft = m2ft(fact.value)
                fact.opts = opts;
                factButton.forceActiveFocus();
            }
            onActiveFocusChanged: {
                if(activeFocus){
                        text=fact.editorText()
                        selectAll();
                }else{
                    text=Qt.binding(function(){return fact.text})
                }
            }
            onFactValueChanged:{
                if(!isFeets)
                    opts.ft = m2ft(fact.value)
                    fact.opts = opts;
            }
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            Rectangle {
                z: parent.z-1
                visible: fact.enabled
                anchors.centerIn: parent
                width: parent.width+Style.spacing*2
                height: parent.height
                radius: height/10
                color: "#000"
                border.width: 0
                opacity: 0.3
            }
        }
    }

    padding: 0 
    spacing: 0
    topPadding: 0
    bottomPadding: 0
    baselineOffset: 0

    background: Item {
        implicitWidth: editor.height*3
    }

    leftPadding: 0
    rightPadding: 0
    leftInset: 0
    rightInset: 0

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth +
                            height * 2 + Style.spacing*4)

    onValueModified: {
        var v=value
        v -= v % stepSize
        v /= div
        if(!isFeets) {
            fact.setValue(v)
            opts.ft = m2ft(v)
            fact.opts = opts
        } else {
            opts.ft = v
            fact.opts = opts
            fact.setValue(ft2m(v))
        }
        value=Qt.binding(function(){return Math.round(!isFeets ? fact.value*div : opts.ft*div)})   
        // accelerate
        elapsed = startTime>0?(new Date().getTime()-startTime)/1000:0
    }


    ToolTip {
        parent: editor
        visible: hovered && tooltip
        font: parent.font
        implicitHeight: parent.height
        topPadding: (implicitHeight-implicitContentHeight)/2
        text: isFeets ? textInput.text : textInputFt.text
        x: parent.x + parent.implicitWidth + 7*ui.scale
        y: parent.y
        background: Rectangle {
            color: "#404040"
            border.color: "#d3d3d3"
            radius: height/10
        }
    }
}
