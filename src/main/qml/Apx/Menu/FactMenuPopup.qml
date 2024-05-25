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

import APX.Facts

import Apx.Common

import "."

Popup {
    id: popup

    property point pos: Qt.point(0.5, 0.5)
    property point posXY: Qt.point(parent.width*pos.x, parent.height*pos.y)
    property bool pinned: false

    property real implicitOpacity: ui.smooth?0.92:1
    property real inactiveOpacity: 0.65

    //forward props
    property alias fact: factMenu.fact
    property bool menuEnabled: true

    function showFact(f)
    {
        raise()
        factMenu.showFact(f)
    }

    function raise()
    {
        Menu.raisePopup(popup)
    }

    opacity: menuEnabled?implicitOpacity:inactiveOpacity

    onAboutToHide: {
        Menu.unregisterMenuView(factMenu)
        Menu.unregisterMenuPopup(popup)

        //destroy immediately on deletion
        if(popup && !popup.fact){
            if(popup && popup.destroy)popup.destroy()
        }
    }
    onOpened: {
        Menu.registerMenuPopup(popup)
    }
    onClosed: {
        if(popup && popup.destroy)popup.destroy()
    }
    //onFactChanged: if(!fact)factMenu.back()

    x: posXY.x - width/2
    y: posXY.y

    padding: 0
    margins: 0

    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: implicitOpacity } }

    closePolicy: pinned?Popup.NoAutoClose:(Popup.CloseOnEscape|Popup.CloseOnPressOutside)

    contentItem: FactMenu {
        id: factMenu
        priority: popup.z
        autoResize: true

        onFactOpened: raise()

        onFactButtonTriggered: (fact) => {
            if(!fact || (fact.options & Fact.CloseOnTrigger))
                back()
            else popup.raise()
        }
        Connections {
            target: factMenu.fact
            function onProgressChanged(){ popup.pinned=true }
        }
        onStackEmpty: popup.close()
        showBtnClose: true
        onCloseTriggered: popup.close()
    }

    //draggable window
    MouseArea {
        id: mouseArea
        // z: popup.menuEnabled?0:(contentItem.z+100)
        anchors.fill: parent
        propagateComposedEvents: popup.menuEnabled
        property point clickPos: Qt.point(0,0)
        onPressed: (mouse) => {
            clickPos = Qt.point(mouse.x,mouse.y)
            popup.raise()
        }
        onPositionChanged: (mouse) => {
            var delta = Qt.point(mouse.x-clickPos.x, mouse.y-clickPos.y)
            popup.x+=delta.x
            popup.y+=delta.y
            popup.pinned=true
        }
        onDoubleClicked: popup.pinned=true
        onPressAndHold: popup.pinned=true
    }
}
