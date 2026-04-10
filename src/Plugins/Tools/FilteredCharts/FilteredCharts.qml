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
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtCore

import Apx.Common

Rectangle {
    id: fcControl

    implicitHeight: layout.implicitHeight
    implicitWidth: layout.implicitWidth
    border.width: 0
    color: "#000"

    Component.onCompleted: {
        for (var i = 0; i < buttonGroup.buttons.length; ++i) {
            var b = buttonGroup.buttons[i];
            if (b.text !== fcControl.currentPage)
                continue;
            buttonGroup.checkedButton = b;
            break;
        }
        if (buttonGroup.checkedButton == null) {
            buttonGroup.checkedButton = buttonGroup.buttons[0]; // check button #1
        }
        loadSettings();
    }

    function saveSettings() {
        var fjson = application.prefs.loadFile("charts.json");
        var json = fjson ? JSON.parse(fjson) : {};
        json.sets = [];
        for (var i = 0; i < buttonGroup.buttons.length; ++i) {
            var b = buttonGroup.buttons[i];
            var set = buttonGroup.buttons[i].getSet();
            if (!set)
                continue;
            json.sets.push(set);
        }
        application.prefs.saveFile("charts.json", JSON.stringify(json, ' ', 2));

        if(fcCharts.resetOn) {
            console.log("fcCharts=", fcCharts.resetOn);  
            fcCharts.resetChartView();
        }
    }

    function loadSettings() {
        var sets = [];
        var fjson = application.prefs.loadFile("charts.json");
        var json = fjson ? JSON.parse(fjson) : {};
        var set = {};
        if (json && json.sets) {
            for (var i in json.sets) {
                set = json.sets[i];
                if (!(typeof set === 'object' && !Array.isArray(set) && set !== null))
                    continue;
                sets.push(set);
            }
        }
        if (sets.length <= 0) {
            return;
        }
        // Create charts
        for (var i in sets) {
            buttonGroup.buttons[i].loadSet(sets[i]);
        }

        fcCharts.resetChartView();
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        FcChartsView {
            id: fcCharts
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130 * ui.scale
        }

        ButtonGroup {
            id: buttonGroup
            onCheckedButtonChanged: fcCharts.resetChartView()
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24 * ui.scale
            FcButton {
                text: "1"
                checked: true
                values: []
            }
            FcButton {
                text: "2"
                values: []
            }
            FcButton {
                text: "3"
                values: []
            }
            FcButton {
                text: "4"
                values: []
            }
            FcButton {
                text: "5"
                values: []
            }
            FcButton {
                text: "6"
                values: []
            }
            FcButton {
                text: "7"
                values: []
            }
            FcButton {
                text: "8"
                values: []
            }
            FcButton {
                text: "9"
                values: []
            }
            FcButton {
                text: "10"
                values: []
            }

            TextButton {
                text: fcCharts.speedFactorValue + "x"
                Layout.fillHeight: true
                Layout.minimumWidth: height * 4
            }
        }
    }

    IconButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Style.spacing
        size: Style.buttonSize * 0.7
        iconName: "plus"
        toolTip: qsTr("Edit charts")
        opacity: ui.effects ? (hovered ? 1 : 0.5) : 1
        onTriggered: {
            var activeButton = buttonGroup.checkedButton;
            activeButton.callQuickChart();
        }
    }

    property string currentPage: buttonGroup.checkedButton.text

    Settings {
        category: "filtredCharts"
        property alias page: fcControl.currentPage
    }
}
