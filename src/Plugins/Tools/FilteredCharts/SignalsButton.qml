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
    property var values: []
    property string pageToolTip: ""
    property bool pageWarning: false

    Layout.fillHeight: true
    checkable: true
    ButtonGroup.group: buttonGroup
    text: pageFact ? pageFact.title.slice(0, 3) : getDefaultText()
    textColor: checked ? Material.color(Material.Yellow) : Material.primaryTextColor
    toolTip: pageToolTip !== "" ? pageToolTip : getToolTip(values)

    onCheckedChanged: if (checked)
        sgCharts.speedFactorValue = sgMenuPage.speed

    onPressed: {
        if (!pageFact)
            return;
        if (checked && !pageFact.active)
            pageFact.trigger();
    }

    onActivated: {
        sgCharts.resetEnable = true;
        sgCharts.facts = Qt.binding(function () {
            return values;
        });
    }

    function getToolTip(facts) {
        var s = [];
        s.push("<strong>" + text + "</strong>");
        for (var i = 0; i < facts.length; ++i) {
            var fact = facts[i];
            if(!fact)
                continue;
            var color = fact.color ? fact.color : (fact.opts ? fact.opts.color : undefined);
            var label = fact.title ? fact.title : (fact.descr ? fact.descr : fact.bind);
            if (color)
                s.push("<font color='" + color + "'>\u25A0</font> " + label);
            else
                s.push(label);
        }
        return s.join("<br>");
    }

    function getDefaultText() {
        return "#" + buttonGroup.buttons.indexOf(this);
    }

    function callQuickChart() {
        sgMenuPage.addNewChart();
    }

    function getSet() {
        return sgMenuPage.save();
    }

    function loadSet(set) {
        sgMenuPage.load(set);
    }

    function getScrMatches(val) {
        return sgMenuPage.checkScrs(val);
    }

    function setSpeed(val) {
        sgMenuPage.speed = val;
        saveTimer.restart(); // save settings after speed changed
    }

    Timer {
        id: timer
        interval: 10000
    }

    Timer {
        id: saveTimer
        interval: 1000
        onTriggered: sgControl.saveSettings()
    }

    SignalsMenuPage {
        id: sgMenuPage
    }
}
