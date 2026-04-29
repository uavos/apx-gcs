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
    readonly property var activeSet: sgMenu.getActiveSet()
    readonly property var activePage: sgMenu.getActivePage()

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

    function checkScrMatches(val) {
        var matches = false;
        for (var i = 0; i < buttonGroup.buttons.length; ++i)
            if (buttonGroup.buttons[i].getScrMatches(val))
                matches = true;
        return matches;
    }

    function changeSpeed() {
        if(!activePage)
            return;
        var newSpeed = 1;
        if (sgMainChart.speedFactorValue !== sgMainChart.speedFactor[sgMainChart.speedFactor.length - 1]) {
            for (var i = 0; i < sgMainChart.speedFactor.length - 1; ++i) {
                if (sgMainChart.speedFactor[i] <= sgMainChart.speedFactorValue && sgMainChart.speedFactorValue < sgMainChart.speedFactor[i + 1]) {
                    activePage.speed = sgMainChart.speedFactor[i + 1]
                    break;
                }
            }
        } else {
            activePage.speed = sgMainChart.speedFactor[0];
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        Item {
            id: mainChartArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130 * ui.scale
            clip: true

            SignalsChartView {
                id: sgMainChart
                anchors.fill: parent
                facts: []
            }
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4 * ui.scale
                radius: 2 * ui.scale
                visible: lblMainPage.text !== ""
                implicitWidth: columnMainPage.implicitWidth + 10 * ui.scale
                implicitHeight: columnMainPage.implicitHeight + 4 * ui.scale
                color: mouseArea.containsMouse ? "#40ffffff" : "#20ffffff"
                opacity: 0.7

                Column {
                    id: columnMainPage
                    anchors.centerIn: parent
                    spacing: 1 * ui.scale

                    Label {
                        id: lblMainPage
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: sgControl.activePage ? sgControl.activePage.title : ""
                        // font: 14 * ui.scale
                    }
                    Label {
                        id: lblMainSpeed
                        anchors.right: parent.right
                        text: sgMainChart.speedFactorValue + "x"
                    }
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    onClicked: changeSpeed()
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 4 * ui.scale
                visible: lblMainSet.text !== ""
                radius: ui.scale
                color: "#30ffffff"
                opacity: 0.7

                implicitWidth: lblMainSet.implicitWidth + 10 * ui.scale
                implicitHeight: lblMainSet.implicitHeight + 4 * ui.scale

                Label {
                    id: lblMainSet
                    anchors.centerIn: parent
                    text: sgControl.activeSet ? sgControl.activeSet.title : ""
                    // font: control.overlayFont(14 * control.uiScale)
                }
            }

            Label {
                anchors.centerIn: parent
                visible: btnRepeater.count <= 0
                text: qsTr("No pages in the active set.")
                color: Material.secondaryTextColor
            }
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

                delegate: SignalsButton {
                    required property int index
                    required property var modelData

                    property int pageIndex: index

                    pageFact: modelData
                    values: pageFact.values

                    // checked: control.selectedPageFact
                    //          ? pageFact === control.selectedPageFact
                    //          : index === 0
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
        }
    }

    SignalsMenu {
        id: sgMenu
    }
}
