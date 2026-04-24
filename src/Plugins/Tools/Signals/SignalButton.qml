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
import QtQuick.Controls.Material

import Apx.Common

TextButton {
    Layout.fillHeight: true
    checkable: true
    ButtonGroup.group: buttonGroup

    property var values: []
    property string pageToolTip: ""
    property bool pageWarning: false

    signal editTriggered()

    textColor: pageWarning ? Material.color(Material.Orange)
                           : Material.primaryTextColor

    onDoubleClicked: editTriggered()
    onPressAndHold: editTriggered()

    toolTip: pageToolTip !== "" ? pageToolTip : getToolTip(values)

    function getToolTip(facts)
    {
        var s=[]
        for(var i=0;i<facts.length;++i){
            var fact=facts[i]
            var color = fact && fact.color ? fact.color
                                           : (fact && fact.opts ? fact.opts.color : undefined)
            var label = fact && fact.title ? fact.title
                                           : (fact && fact.descr ? fact.descr : fact.bind)
            if(color)
                s.push("<font color='"+color+"'>"+label+"</font>")
            else
                s.push(label)
        }
        return s.join("<br>")
    }
}
