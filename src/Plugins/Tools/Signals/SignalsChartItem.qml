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

import QtCharts
import QtQml

import Apx.Common


Item {
    id: chartItem
    
    property var ciPageFact: null
    property var num: ciPageFact ? ciPageFact.num : 0
    property var labelFont: apx.font_narrow(Style.fontSize * 0.8) 

    SignalsChartView {
        id: ciChart
        anchors.fill: parent
        facts: ciPageFact ? ciPageFact.values : []
        speedFactorValue: ciPageFact ? ciPageFact.speed : 1
    }

    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4 * ui.scale
        radius: 2 * ui.scale
        visible: lblPage.text !== ""
        color: mouseArea.containsMouse ? "#50ffffff" : "#30ffffff"
        implicitWidth: columnPage.implicitWidth + 10 * ui.scale
        implicitHeight: columnPage.implicitHeight + 4 * ui.scale

        Column {
            id: columnPage
            anchors.centerIn: parent
            opacity: 0.7

            Label {
                id: lblPage
                anchors.horizontalCenter: parent.horizontalCenter
                text: ciPageFact ? ciPageFact.title : ""
                font: labelFont
            }
            Label {
                id: lblSpeed
                anchors.right: parent.right
                text: ciChart.speedFactorValue + "x"
                font: labelFont
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

    function changeSpeed() {
        if (!ciPageFact)
            return;
        if (ciChart.speedFactorValue !== ciChart.speedFactor[ciChart.speedFactor.length - 1]) {
            for (var i = 0; i < ciChart.speedFactor.length - 1; ++i) {
                if (ciChart.speedFactor[i] <= ciChart.speedFactorValue && ciChart.speedFactorValue < ciChart.speedFactor[i + 1]) {
                    ciPageFact.speed = ciChart.speedFactor[i + 1];
                    break;
                }
            }
        } else {
            ciPageFact.speed = ciChart.speedFactor[0];
        }
        autosaveTimer.restart();
    }

    function allowReset() {
        ciChart.resetEnable = true;
    }
}
