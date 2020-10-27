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
import QtQuick 2.2
import "../common"
import "."

Item {
    id: wind_window
    readonly property bool m_wind: mandala.est.wind.status.value > 0
    readonly property real m_wspd: mandala.est.wind.speed.value
    readonly property real m_whdg: mandala.est.wind.heading.value

    property real value: m_whdg

    property real anumation_duration: 1000
    property bool simplified: false

    visible: ui.test || m_wind || m_wspd > 0

    readonly property color color: m_wind?"#fff":"yellow"

    PfdImage {
        id: wind_arrow
        anchors.fill: parent
        anchors.rightMargin: parent.width-parent.width*parent.height/parent.width
        anchors.centerIn: parent
        anchors.margins: simplified?1:0
        elementName: simplified?"wind-arrow-simple":"wind-arrow"
        //smooth: ui.antialiasing
        fillMode: Image.PreserveAspectFit
        rotation: value
        Behavior on rotation { enabled: ui.smooth; RotationAnimation {duration: anumation_duration; direction: RotationAnimation.Shortest; } }
    }
    Item {
        anchors.fill: wind_arrow
        rotation: wind_arrow.rotation
        visible: !simplified
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -wind_arrow.height*0.7
            rotation: -parent.rotation
            text: m_wspd.toFixed(m_wspd>=10?0:1)
            color: wind_window.color
            font.family: font_narrow
            font.pixelSize: parent.height*0.5
        }
    }
}
