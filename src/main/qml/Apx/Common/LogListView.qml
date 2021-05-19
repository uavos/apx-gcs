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

import "."

ListView {
    id: listView

    signal clicked()

    clip: true
    spacing: Style.spacing/2

    focus: false
    keyNavigationEnabled: false

    ScrollBar.vertical: ScrollBar {
        width: Style.buttonSize/4
        active: !listView.atYEnd
    }
    boundsBehavior: Flickable.StopAtBounds
    
    readonly property bool scrolling: dragging||flicking
    onScrollingChanged: stickEnd=false
    
    property bool stickEnd: false

    Component.onCompleted: scrollToEnd()
    function scrollToEnd()
    {
        positionViewAtEnd()
        stickEnd=true
    }
    Timer {
        id: scrollEndTimer
        interval: 1
        onTriggered: if(listView.stickEnd) listView.scrollToEnd()
    }

    Connections {
        target: listView.model
        function onRowsInserted() {
            if(listView.stickEnd) {
                listView.scrollToEnd()
                scrollEndTimer.start()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            listView.scrollToEnd()
            listView.clicked()
        }
    }
}
