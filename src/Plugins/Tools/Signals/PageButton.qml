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

ValueButton {
    id: pageBtn

    Layout.fillHeight: true
    checkable: true
    ButtonGroup.group: pageButtonGroup
    showValue: false
    alerts: true

    // The MenuPage Fact this button represents
    property var page: null

    warning: page && page.hasWarning && !page.hasAlarm
    error: page && page.hasAlarm
    active: checked
    normalColor: "#222"
    activeColor: Qt.darker(Material.color(Material.BlueGrey), 1.5)

    // Pinned indicator — a subtle border
    background: Rectangle {
        color: checked
               ? Qt.darker(Material.color(Material.BlueGrey), 1.5)
               : "transparent"
        border.width: (page && page.pinned) ? 1 : 0
        border.color: Material.color(Material.Cyan)
        radius: height / 6
    }

    descr: page && page.warningText ? page.warningText : ""

    toolTip: buildToolTip()

    function buildToolTip() {
        if (!page)
            return text;
        var s = [];
        s.push("<strong>" + page.title + "</strong>");
        if (page.warningText && page.warningText !== "") {
            var warnColor = page.hasAlarm ? Material.color(Material.Red) : Material.color(Material.Orange);
            s.push("<font color='" + warnColor + "'>" + page.warningText + "</font>");
        }
        var values = page.values;
        for (var i = 0; i < values.length; ++i) {
            var it = values[i];
            s.push("<font color='" + it.itemColor + "'>" + it.itemTitle + "</font>");
        }
        if (page.pinned)
            s.push(qsTr("(pinned)"));
        return s.join("<br>");
    }
}
