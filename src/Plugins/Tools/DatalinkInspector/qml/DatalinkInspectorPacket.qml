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

RowLayout {
    id: control

    //prefs
    property int fontSize: 12


    //model data
    property alias packet: _repeater.model

    spacing: Style.spacing/2

    /*Label {
        Layout.alignment: Qt.AlignLeft|Qt.AlignVCenter
        Layout.fillHeight: true
        text: uplink?">":"<"
        color: uplink?"#fff":"cyan"
        font.family: font_condenced
        font.pixelSize: fontSize
    }*/

    function get_color(text)
    {
        if(text.startsWith(">")) return "#fff"
        if(text.startsWith("<")) return "cyan"
        if(text.startsWith("$")) return "#ffa"
        if(text.startsWith("+")) return "#aaf"
        if(text.endsWith(":")) return "#afa"
        /*switch(type){
        case Protocols.PACKET: return uplink?"#aaa":"cyan"
        case Protocols.NMT: return "#faa"
        case Protocols.PID: return "#ffa"
        case Protocols.GUID: return "#a88"
        }*/
        return "#aaa"
    }
    function get_text(text)
    {
        if(text.startsWith("$")) return text.slice(1)
        return text
    }

    property bool uplink: packet[0].startsWith(">")


    Repeater {
        id: _repeater

        DatalinkInspectorItem {
            id: item
            Layout.alignment: Qt.AlignLeft|Qt.AlignVCenter
            Layout.fillHeight: true
            text: get_text(modelData)
            itemColor: get_color(modelData)
        }
    }
    Item {
        implicitHeight: 1
        implicitWidth: 1
        Layout.fillWidth: true
    }
}
