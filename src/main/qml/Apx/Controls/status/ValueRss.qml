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

import Apx.Common

ValueButton {
    id: control

    fact: apx.datalink
    enabled: true

    active: false

    text: qsTr("RSS")

    readonly property var f_rss: mandala.sns.com.rss
    readonly property real m_rss: f_rss?f_rss.value:0

    readonly property var f_gcs_rss: apx.fleet.gcs?apx.fleet.gcs.mandala.sns.com.rss:null
    readonly property real m_gcs_rss: f_gcs_rss?f_gcs_rss.value:0


    toolTip: f_rss?f_rss.descr+ "\n" + qsTr("Onboard") + "/" + qsTr("Ground"):""

    property real value1: Math.max(0, Math.min(1, m_rss/100))
    property real value2: Math.max(0, Math.min(1, m_gcs_rss/100))

    property color barColorBG: "#555"

    property int barHeight: control.height/2*0.5

    function barColor(v)
    {
        return v<0.3?"red":v<0.7?"yellow":"green"
    }


    valueC: Component {
        Item {
            implicitWidth: control.height*2
            Rectangle {
                anchors.bottom: parent.verticalCenter
                anchors.bottomMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                implicitHeight: barHeight
                border.width: 0
                color: barColorBG
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: parent.width-parent.width*value1
                    border.width: 0
                    color: barColor(value1)
                }
            }
            Rectangle {
                anchors.top: parent.verticalCenter
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                implicitHeight: barHeight
                border.width: 0
                color: barColorBG
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: parent.width-parent.width*value2
                    border.width: 0
                    color: barColor(value2)
                }
            }
        }
    }
}
