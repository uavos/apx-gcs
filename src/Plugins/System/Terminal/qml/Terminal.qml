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
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

import APX.Facts 1.0

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: 600
    implicitHeight: 300

    property real lineSize: Style.buttonSize*0.4
    property real lineSpace: 0

    ColumnLayout {
        spacing: 0
        anchors.margins: Style.spacing
        anchors.fill: parent


        ListView { // TODO better handling of max lines and auto scroll
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignBottom
            Layout.preferredWidth: 300
            Layout.preferredHeight: 400

            clip: true

            spacing: lineSpace

            model: application.notifyModel
            delegate: TerminalLine {
                width: listView.width
                text: model.text
                subsystem: model.subsystem
                source: model.source
                type: model.type
                options: model.options
                fact: model.fact
                timestamp: model.timestamp
            }

            add: Transition {
                id: transAdd
                enabled: ui.smooth
                NumberAnimation {
                    properties: "x";
                    from: -listView.width;
                    duration: transAdd.ViewTransition.item.source==AppNotify.FromInput?0:150
                    easing.type: Easing.OutCubic
                }
            }


            ScrollBar.vertical: ScrollBar {
                width: Style.spacing*2
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

            footer: TerminalExec {
                width: parent.width
                onFocused: listView.scrollToEnd()
            }

            function scrollToEnd()
            {
                positionViewAtEnd()
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listView.footerItem.focusRequested()
                    listView.scrollToEnd()
                }
            }
        }
    }
}
