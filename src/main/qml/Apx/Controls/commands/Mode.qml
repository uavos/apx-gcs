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
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

ListView {
    id: listView

    readonly property var f_mode: mandala.cmd.proc.mode

    implicitHeight: 32

    property var modes: []

    orientation: ListView.Horizontal
    spacing: 8
    model: modes
    delegate: ToolButton {
        size: listView.height
        text: modelData
        toolTip: f_mode.title+": "+modelData
        iconName: listView.modeIcon(modelData)
        onTriggered: {
            f_mode.value=modelData
        }
    }
    headerPositioning: ListView.OverlayHeader
    header: TextButton {
        size: listView.height
        minimumWidth: size*3
        text: f_mode.text
        property int v: f_mode.value
        property bool warning: v==proc_mode_EMG || v==proc_mode_RPV || v==proc_mode_TAXI
        property bool active: v==proc_mode_LANDING || v==proc_mode_TAKEOFF
        color: "#000"
        textColor: warning?Material.color(Material.Yellow):active?Material.color(Material.Blue):Qt.darker(Material.primaryTextColor,1.5)
        onTriggered: {
            popupC.createObject(this)
        }
    }


    function modeIcon(mode)
    {
        switch(mode)
        {
        default: return ""
        case "EMG": return "google-controller"
        case "RPV": return "google-controller"
        case "UAV": return "directions-fork"
        case "WPT": return "navigation"
        case "STBY": return "clock-in"
        case "TAXI": return "taxi"
        case "TAKEOFF": return "airplane-takeoff"
        case "LANDING": return "airplane-landing"
        }
    }

    Component{
        id: popupC
        Popup {
            id: popup
            width: 150 //contentItem.implicitWidth
            height: contentItem.implicitHeight //Math.min(listView.implicitHeight, control.implicitHeight - topMargin - bottomMargin)
            topMargin: 6
            bottomMargin: 6
            padding: 0
            margins: 0
            x: parent.width

            Component.onCompleted: open()
            onClosed: destroy()

            contentItem: ListView {
                id: listView
                implicitHeight: contentHeight
                implicitWidth: contentWidth
                model: f_mode.enumStrings
                highlightMoveDuration: 0
                delegate: ItemDelegate {
                    text: modelData
                    width: Math.max(listView.width,implicitWidth)
                    highlighted: text===f_mode.text
                    onClicked: {
                        popup.close()
                        f_mode.value=text
                    }
                }
                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }
}

