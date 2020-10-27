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
import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import Apx.Common 1.0

Item {
    id: control
    implicitHeight: factMenu.MenuStyle.itemSize
    implicitWidth: factMenu.MenuStyle.itemWidth
    clip: true
    Text {
        id: titleText
        z: 10
        anchors.top: parent.top
        anchors.left: showBtnBack?btnBack.right:parent.left
        anchors.leftMargin: 8
        font.pixelSize: Math.max(8,parent.height*(titleDescr.visible?0.65:0.8))
        font.family: font_narrow
        color: "white"
        visible: text!=""
        text: pageTitle
    }
    Text {
        id: titleDescr
        anchors.bottom: parent.bottom
        anchors.left: titleText.left
        anchors.bottomMargin: 1
        font.pixelSize: Math.max(8,parent.height*0.35)
        font.family: font_condenced
        color: "#aaa"
        visible: text!=""
        text: pageDescr
    }
    Text {
        id: statusText
        anchors.verticalCenter: titleText.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: titleRightMargin+8
        font: titleText.font
        color: "#aaa"
        visible: text!=""
        text: pageStatus
    }
    FastBlur {
        anchors.fill: titleText
        transparentBorder: true
        source: titleText
        radius: ui.antialiasing?titleText.height/2:0
        visible: factMenu && factMenu.effects && titleText.visible
    }
    IconButton {
        id: btnBack
        visible: showBtnBack
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -2
        iconName: "chevron-left"
        size: parent.height*0.8
        onClicked: back()
        color: "#478fff"
    }
    Rectangle {
        id: bottomBar
        height: 2
        color: MenuStyle.cTitleSep
        border.width: 0
        Component.onCompleted: {
            anchors.left=control.left
            anchors.right=control.right
            anchors.bottom=control.bottom
        }
    }
    Loader {
        id: progress
        anchors.left: control.left
        anchors.right: control.right
        anchors.bottom: control.bottom
        height: bottomBar.height
        property int v: fact.progress
        active: v>=0
        visible: active
        ProgressBar {
            height: bottomBar.height
            width: control.width
            to: 100
            value: progress.v
            indeterminate: progress.v==0
            Material.accent: Material.color(Material.LightGreen)
        }
    }
}
