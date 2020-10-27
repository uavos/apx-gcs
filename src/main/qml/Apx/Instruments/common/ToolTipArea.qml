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
import QtQuick.Controls 2.3

MouseArea {
    id: mouseArea
    //property alias tip: tip
    //propagateComposedEvents: true
    //preventStealing: true
    property string text    // tip.text
    //property alias hideDelay: hideTimer.interval
    //property alias showDelay: showTimer.interval
    //property bool tipVisible: false
    acceptedButtons: Qt.NoButton
    anchors.fill: parent
    hoverEnabled: true
    /*function show(){
        visible = true;
        apx.toolTip(text);
    }*/
    /*Timer {
        id:showTimer
        interval: 1000
        running: mouseArea.containsMouse && !tipVisible && text.length>0
        onTriggered: show();
    }
    Timer {
        id:hideTimer
        interval: 100
        running: !mouseArea.containsMouse && tipVisible
        onTriggered: tipVisible=false;  //tip.hide();
    }*/
    /*ToolTip{
        id:tip
    }*/
    ToolTip {
        delay: 1000
        timeout: 5000
        visible: containsMouse
        text: mouseArea.text
    }
}

