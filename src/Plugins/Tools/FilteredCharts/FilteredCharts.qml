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
    id: sgControl

    implicitHeight: layout.implicitHeight
    implicitWidth: layout.implicitWidth
    border.width: 0
    color: "#000"

    readonly property var pages: sgMenu.getActivePages()

    Component.onCompleted: {
        for (var i = 0; i < buttonGroup.buttons.length; ++i) {
            var b = buttonGroup.buttons[i];
            if (b.text !== sgControl.currentPage)
                continue;
            buttonGroup.checkedButton = b;
            break;
        }
        //if (buttonGroup.checkedButton == null) {
        //    buttonGroup.checkedButton = buttonGroup.buttons[0]; // check button #1
        //}
        loadSettings();
    }

    Connections {
        target: apx.fleet.current.mandala
        function onTelemetryDecoded() {
            if (pages.length <= 0)
                return;
            for (var i = 0; i < pages.length; ++i)
                pages[i].updateChartsValues();
        }
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
            if (i < buttonGroup.buttons.length) {
                buttonGroup.buttons[i].loadSet(sets[i]);
            }
        }
    }

    function checkScrMatches(val) {
        var matches = false;
        for (var i = 0; i < buttonGroup.buttons.length; ++i)
            if (buttonGroup.buttons[i].getScrMatches(val))
                matches = true;
        return matches;
    }

    function changeSpeed() {
        if (sgCharts.speedFactorValue !== sgCharts.speedFactor[sgCharts.speedFactor.length - 1]) {
            for (var i = 0; i < sgCharts.speedFactor.length - 1; ++i) {
                if (sgCharts.speedFactor[i] <= sgCharts.speedFactorValue && sgCharts.speedFactorValue < sgCharts.speedFactor[i + 1]) {
                    buttonGroup.checkedButton.setSpeed(sgCharts.speedFactor[i + 1]);
                    break;
                }
            }
        } else {
            buttonGroup.checkedButton.setSpeed(sgCharts.speedFactor[0]);
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        SignalsChartView {
            id: sgCharts
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130 * ui.scale
        }

        ButtonGroup {
            id: buttonGroup
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24 * ui.scale
            Repeater {
                id: btnRepeater
                model: sgControl.pages

                // onItemAdded: function(index, item) {
                //     if (index === tabsRepeater.count - 1)
                //         Qt.callLater(control.selectSavedPage)
                // }

                delegate: SignalsButton {
                    required property int index
                    required property var modelData

                    property int pageIndex: index

                    pageFact: modelData
                    values: pageFact.values

                    //text: pageFact.title.slice(0, 3)

                    // checked: control.selectedPageFact
                    //          ? pageFact === control.selectedPageFact
                    //          : index === 0
                    // values: control.pageFacts(index)
                    // pageToolTip: control.pageState(index).toolTip
                    // pageWarning: control.pageState(index).warning
                    // onEditTriggered: control.openPageEditor(pageFact)
                }
            }
            IconButton {
                iconName: "plus"
                toolTip: qsTr("Edit chart configuration")
                onClicked: sgMenu.trigger()
                // SignalsMenu {
                //     id: sgMenu
                // }
            }

            TextButton {
                text: sgCharts.speedFactorValue + "x"
                Layout.fillHeight: true
                Layout.minimumWidth: height * 4
                onClicked: changeSpeed()
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

    SignalsMenu {
        id: sgMenu
    }
}
