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
    
    onActiveSetChanged: updateModels()
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

    function updateModels() {
        pinnedModel.updateModel(sgMenu.getPinnedPages())
        buttonsModel.updateModel(sgMenu.getActivePages())
    }

    function checkScrMatches(val) {
        var matches = false;
        for (var i = 0; i < buttonGroup.buttons.length; ++i)
            if (buttonGroup.buttons[i].getScrMatches(val))
                matches = true;
        return matches;
    }

    function allowResetChart(num) {
        for(var i = 0; i < pinnedRepeater.count; ++i) {
            if(num === pinnedRepeater.itemAt(i).num)
                pinnedRepeater.itemAt(i).allowReset()
        }
    }

    function clearButtonGroup() {
        buttonGroup.buttons = [];
        buttonGroup.checkedButton = null;
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent

        SignalsPinnedModel {
            id: pinnedModel
        }

        Repeater {
            id: pinnedRepeater
            model: pinnedModel
        }

        SignalsChartItem {
            id: mainChartArea

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 110 * ui.scale // 130 * ui.scale
            clip: true

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
                    font: mainChartArea.labelFont
                }
            }

            Label {
                anchors.centerIn: parent
                visible: btnRepeater.count <= 0
                text: qsTr("No pages in the active set")
                font: mainChartArea.labelFont
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
                mainChartArea.allowReset();    
                mainChartArea.ciPageFact = null;
            }
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            Layout.maximumHeight: 24 * ui.scale
            spacing: 3
            
            SignalsButtonsModel {
               id: buttonsModel
            }
            Repeater {
                id: btnRepeater
                model: buttonsModel
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
