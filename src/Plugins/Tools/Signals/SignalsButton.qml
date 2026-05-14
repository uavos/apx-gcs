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
    id: sgBtn

    property var pageFact: null
    property string pageToolTip: pageFact ? pageFact.pageToolTip : ""
    property bool pageWarning: pageFact ? pageFact.warnings : false
    property var num: pageFact ? pageFact.num : -1

    Layout.fillHeight: true
    checkable: true
    ButtonGroup.group: buttonGroup
    text: pageFact ? pageFact.title.slice(0, 3) : getDefaultText()
    textColor: checked ? Material.color(Material.Yellow) : Material.primaryTextColor
    toolTip: pageFact ? pageFact.pageToolTip : ""
    Material.background: pageWarning ? Material.color(Material.Orange) : color

    Component.onCompleted: {
        if(!pageFact)
            return;
        checked = pageFact.active    
    } 
    onCheckedChanged: {
        if (!pageFact)
            return;
        if (checked)
            mainChartArea.ciPageFact = pageFact;
        pageFact.active = checked;
    }

    onPressed: {
        if (!pageFact)
            return;
        if (checked)
            pageFact.trigger();
    }

    onActivated: {
        mainChartArea.allowReset()
        mainChartArea.ciPageFact = Qt.binding(function () {
            return pageFact;
        });
    }

    function getDefaultText() {
        return "#" + buttonGroup.buttons.indexOf(this);
    }
}
