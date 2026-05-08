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
    implicitWidth: Style.buttonSize * 15
    border.width: 0
    color: "#000"

    readonly property var pages: sgMenu.getActivePages()
    readonly property var activeSet: sgMenu.getActiveSet()
    readonly property var activePage: sgMenu.getActivePage()
    readonly property var pinnedPages: sgMenu.getPinnedPages()

    Component.onCompleted: {
        if (buttonGroup.buttons.length <= 0)
            return;
        if (buttonGroup.checkedButton == null)
            buttonGroup.checkedButton = buttonGroup.buttons[0]; // check button #1
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
        if (!activePage)
            return;
        if (sgMainChart.speedFactorValue !== sgMainChart.speedFactor[sgMainChart.speedFactor.length - 1]) {
            for (var i = 0; i < sgMainChart.speedFactor.length - 1; ++i) {
                if (sgMainChart.speedFactor[i] <= sgMainChart.speedFactorValue && sgMainChart.speedFactorValue < sgMainChart.speedFactor[i + 1]) {
                    activePage.speed = sgMainChart.speedFactor[i + 1];
                    break;
                }
            }
        } else {
            activePage.speed = sgMainChart.speedFactor[0];
        }
        autosaveTimer.restart();
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent

        property var chartLabelFont: apx.font_narrow(Style.fontSize * 0.8) 

        Repeater {
            model: sgControl.pinnedPages

            delegate: Item {
                id: pinnedChartArea

                required property var modelData
                required property int index

                property var pinnedPageFact: modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 110 * ui.scale
                Layout.minimumHeight: 20

                SignalsChartView {
                    id: sgPinnedChart
                    anchors.fill: parent
                    facts: pinnedPageFact ? pinnedPageFact.values : []
                    speedFactorValue: pinnedPageFact ? pinnedPageFact.speed : 1
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 4 * ui.scale
                    radius: 2 * ui.scale
                    visible: lblPinnedPage.text !== ""
                    color: maPinned.containsMouse ? "#50ffffff" : "#30ffffff"
                    implicitWidth: columnPinnedPage.implicitWidth + 10 * ui.scale
                    implicitHeight: columnPinnedPage.implicitHeight + 4 * ui.scale

                    Column {
                        id: columnPinnedPage
                        anchors.centerIn: parent
                        Label {
                            id: lblPinnedPage
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: pinnedPageFact ? pinnedPageFact.title : ""
                            font: layout.chartLabelFont
                        }
                        Label {
                            id: lblPinnedSpeed
                            anchors.right: parent.right
                            text: sgPinnedChart.speedFactorValue + "x"
                            font: layout.chartLabelFont
                        }
                    }
                    MouseArea {
                        id: maPinned
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        onClicked: changePinnedSpeed()
                    }
                }

                function changePinnedSpeed() {
                    if (!pinnedPageFact)
                        return;
                    if (sgPinnedChart.speedFactorValue !== sgPinnedChart.speedFactor[sgPinnedChart.speedFactor.length - 1]) {
                        for (var i = 0; i < sgPinnedChart.speedFactor.length - 1; ++i) {
                            if (sgPinnedChart.speedFactor[i] <= sgPinnedChart.speedFactorValue && sgPinnedChart.speedFactorValue < sgPinnedChart.speedFactor[i + 1]) {
                                pinnedPageFact.speed = sgPinnedChart.speedFactor[i + 1];
                                break;
                            }
                        }
                    } else {
                        pinnedPageFact.speed = sgMainChart.speedFactor[0];
                    }
                    autosaveTimer.restart();
                }
            }
        }

        Item {
            id: mainChartArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 110 * ui.scale // 130 * ui.scale
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
                color: maMain.containsMouse ? "#50ffffff" : "#30ffffff"
                opacity: 0.7

                Column {
                    id: columnMainPage
                    anchors.centerIn: parent
                    Label {
                        id: lblMainPage
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: sgControl.activePage ? sgControl.activePage.title : ""
                        font: layout.chartLabelFont
                    }
                    Label {
                        id: lblMainSpeed
                        anchors.right: parent.right
                        text: sgMainChart.speedFactorValue + "x"
                        font: layout.chartLabelFont
                    }
                }
                MouseArea {
                    id: maMain
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
                    font: apx.font_narrow(Style.fontSize * 0.8)
                }
            }

            Label {
                anchors.centerIn: parent
                visible: btnRepeater.count <= 0
                text: qsTr("No pages in the active set")
                font: apx.font_narrow(Style.fontSize * 0.8)
                color: Material.secondaryTextColor
                background: Rectangle {
                    color: sgControl.color
                }
            }
        }

        ButtonGroup {
            id: buttonGroup

            onCheckedButtonChanged: {
                if (checkedButton)
                    return;
                sgMainChart.resetEnable = true;    
                sgMainChart.facts = []
            }
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
                }
            }
            IconButton {
                iconName: "plus"
                toolTip: qsTr("Edit chart configuration")
                Layout.fillHeight: true
                Layout.minimumWidth: height
                onClicked: sgMenu.trigger()
            }
        }
    }

    SignalsMenu {
        id: sgMenu
    }

    Timer {
        id: autosaveTimer
        interval: 1000
        onTriggered: sgMenu.saveSettings()
    }

    Timer {
        id: warnTimer
        interval: 10000
    }
}
