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
import QtQuick 2.2

Item {
    id: stripRect
    property double value
    property double numScale: 0.5
    property double numGapScale: 1
    property color color: "white"
    property color colorNeg: "#f88"
    property bool showZero: true
    property int divider: 0


    Rectangle {
        color: "transparent" //"#5000ff00" //
        border.width: 0
        anchors.fill: parent
        clip: true
        Item {
            id: strip
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: offset(dspValue)*numStep
            property double dspValue: divider>0?(value>=0?Math.floor((value+0.4)/divider):Math.ceil((value-0.4)/divider)):value
            property double numSize: stripRect.height*stripRect.numScale
            property double numStep: numSize*stripRect.numGapScale

            Behavior on dspValue { enabled: ui.smooth && divider>0; PropertyAnimation {duration: 100; } }

            function mod(x,y) {
                return x-y*Math.floor(x/y);
            }
            function offset(v) {
                var x=mod(v,10);
                if(v<-10) x-=20;
                else if(v<0) x-=10;
                else if(v>=10)x+=10;
                return x;
            }
            Repeater {
                model: 30
                Text {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -(index)*strip.numStep
                    text: (showZero===false && strip.dspValue<5 && strip.dspValue>-5 && (index%10)===0)?"":Math.abs(index) % 10
                    color: stripRect.color
                    font: apx.font_narrow(strip.numSize)
                }
            }
            Repeater {
                model: 30
                Text {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: (index+1)*strip.numStep
                    text: (showZero===false && strip.dspValue<5 && strip.dspValue>-5 && ((index+1)%10)===0)?"":Math.abs(index+1) % 10
                    color: stripRect.colorNeg
                    font: apx.font_narrow(strip.numSize)
                }
            }
        }
    }
}
