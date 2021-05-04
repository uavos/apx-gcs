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
    clip: true

    implicitHeight: buttonSize + Style.spacing * 2
    implicitWidth: factMenu.MenuStyle.itemWidth

    property real buttonSize: Style.buttonSize

    Text {
        id: titleText
        z: 10
        anchors.top: parent.top
        anchors.left: showBtnBack?btnBack.right:parent.left
        anchors.leftMargin: Style.spacing*2
        anchors.topMargin: -Style.spacing
        font: apx.font_narrow(parent.height*(titleDescr.visible?0.65:0.8))
        color: "white"
        visible: text!=""
        text: pageTitle
    }
    Text {
        id: titleDescr
        anchors.top: titleText.bottom
        anchors.left: titleText.left
        anchors.right: showBtnClose?btnClose.left:parent.right
        anchors.rightMargin: (statusText.truncated?statusText.width:statusText.implicitWidth)+Style.spacing*2
        anchors.topMargin: -Style.spacing*2
        font: apx.font_narrow(parent.height*0.35)
        color: "#aaa"
        visible: text!=""
        text: pageDescr
        elide: Text.ElideRight
    }
    Text {
        id: statusText
        anchors.right: showBtnClose?btnClose.left:parent.right
        anchors.rightMargin: Style.spacing*2
        anchors.left: titleText.right
        anchors.leftMargin: Style.spacing*2
        horizontalAlignment: Text.AlignRight
        font: titleText.font
        color: "#aaa"
        visible: text!=""
        text: pageStatus
        elide: Text.ElideMiddle
    }
    FastBlur {
        anchors.fill: titleText
        transparentBorder: true
        source: titleText
        radius: ui.antialiasing?titleText.height/2:0
        visible: factMenu && factMenu.effects && titleText.visible
    }

    // Buttons
    IconButton {
        id: btnBack
        visible: showBtnBack
        anchors.left: parent.left
        anchors.top: parent.top
        iconName: "chevron-left"
        size: buttonSize
        onClicked: back()
        color: "#478fff"
    }
    IconButton {
        id: btnClose
        z: 10
        visible: showBtnClose
        anchors.right: parent.right
        anchors.top: parent.top
        iconName: "close"
        color: (popup && popup.pinned)?Material.BlueGrey:undefined
        size: buttonSize
        onClicked: closeTriggered()
    }

    // Bottom bar and progress
    Rectangle {
        id: bottomBar
        height: Style.spacing
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
