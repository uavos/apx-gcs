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
import QtQuick.Controls.Material

import APX.Facts

Fact {
    id: mColor

    readonly property var colorAuto: qsTr("Auto")
    property bool changes: false
    
    readonly property var colorBaseLabels: [
        qsTr("Red"),
        qsTr("Pink"),
        qsTr("Purple"),
        qsTr("Deep Purple"),
        qsTr("Indigo"),
        qsTr("Blue"),
        qsTr("Light Blue"),
        qsTr("Cyan"),
        qsTr("Teal"),
        qsTr("Green"),
        qsTr("Orange"),
        qsTr("Blue Grey")
    ]
    readonly property var colorBaseValues: [
        Material.Red,
        Material.Pink,
        Material.Purple,
        Material.DeepPurple,
        Material.Indigo,
        Material.Blue,
        Material.LightBlue,
        Material.Cyan,
        Material.Teal,
        Material.Green,
        Material.Orange,
        Material.BlueGrey
    ]
    readonly property var colorShadeLabels: [
        qsTr("300"),
        qsTr("500"),
        qsTr("700"),
        qsTr("900")
    ]
    readonly property var colorShadeValues: [
        Material.Shade300,
        Material.Shade500,
        Material.Shade700,
        Material.Shade900
    ]
  
    onValueChanged: updateChartColor()
    onChangesChanged: { if (changes) mChart.changes = true;}
    Component.onCompleted: {
        if (!value || value === undefined) 
            value = colorAuto
        if(!mChart.newItem && value ===  colorAuto)
            setDefaultColor()
        rebuildColorChoices()
    }

    function clearColorChoices()
    {
        if (!mColor)
            return
        for (var i = mColor.size - 1; i >= 0; --i) {
            var child = mColor.child(i)
            if (child) {
                child.deleteFact()
            }
        }
    }

    function rebuildColorChoices()
    {
        clearColorChoices();
        createFact(mColor, "SignalsColorChooser.qml", {
                       "title": colorAuto,
                       "descr": qsTr("Use automatic series color"),
                       "colorValue": "",
                       "section": "",
                       "value": colorAuto
                   })
        for (var i = 0; i < colorShadeValues.length; ++i) {
            var shadeLabel = colorShadeLabels[i]
            for (var j = 0; j < colorBaseValues.length; ++j) {
                var colorCode = Material.color(colorBaseValues[j],
                                               colorShadeValues[i]).toString().toUpperCase()
                createFact(mColor, "SignalsColorChooser.qml", {
                               "title": colorBaseLabels[j],
                               "descr": colorCode,
                               "colorValue": colorCode,
                               "section": shadeLabel,
                               "value": colorCode
                           })
            }
        }
    }

    function createFact(parent, url, opts) {
        var component = Qt.createComponent(url);
        if (component.status === Component.Ready) {
            var c = component.createObject(parent, opts);
            c.parentFact = parent;
            return c;
        }
    }

    function setDefaultColor()
    {
        var colorsCount = colorBaseValues.length
        if (colorsCount <= 0) {
            value = "#FFFFFF";
            return;
        }
        var index = 0;
        if (mCharts.size !== 0) {
            if(mChart.num === 0) {
                var f = mChart.parentFact
                index = !(f && f.title === "Charts") ? mCharts.size % colorsCount : 0
            } else {
                index = mChart.num % colorsCount
            }
        }
        value = Material.color(colorBaseValues[index], colorShadeValues[0]).toString().toUpperCase();
    }

    function updateChartColor() {
        if(!mChart.newItem)
            mChart.updateIconColor(value)
        updateDescr();
    }

    function updateDescr() {
        var descrText = "";
        if(!value || value === undefined)
            descrText = colorAuto
        else
            descrText = value
        descr = qsTr("Color").toUpperCase() + ": " + descrText.toString().toUpperCase();
        changes = true;
    }
}
