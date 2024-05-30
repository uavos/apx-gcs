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
    
    from: (typeof fact.min!=='undefined')?fact.min*div:-1000000000
    to: (typeof fact.max!=='undefined')?fact.max*div:1000000000
    
    property real div: 1

    value: fact.value * div

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


    contentItem: Item{
        implicitWidth: textInput.width
        TextInput {
            id: textInput
            anchors.centerIn: parent
            font: editor.font

            color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
            text: fact.text

            activeFocusOnTab: true

            width: Math.max(contentWidth, height*3)

            selectByMouse: true
            onEditingFinished: {
                fact.setValue(text);
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

        fact.setValue(v)

        value=Qt.binding(function(){return Math.round(fact.value*div)})

        // accelerate
        elapsed = startTime>0?(new Date().getTime()-startTime)/1000:0
    }

}
