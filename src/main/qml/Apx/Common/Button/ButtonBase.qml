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
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Button {
    id: control

    property string toolTip

    property var color

    property int size: 32 * ui_scale

    property int minimumWidth: 0

    property real ui_scale: ui.scale

    signal triggered()
    signal activated()

    focus: false
    font.capitalization: Font.MixedCase

    // geometry

    padding: height/32
    spacing: height/20
    topInset: 0
    bottomInset: 1

    leftPadding: padding+1
    rightPadding: padding+2
    topPadding: padding
    bottomPadding: padding+1

    implicitHeight: size
    implicitWidth: defaultWidth

    property int defaultWidth: Math.max(height, Math.max(minimumWidth, implicitContentWidth + leftPadding+rightPadding))

    // colors
    highlighted: activeFocus
    Material.background: color ? color : undefined
    Material.theme: Material.Dark
    Material.accent: Material.color(Material.BlueGrey)
    Material.primary: Material.color(Material.LightGreen)

    // tooltip

    //ToolTip.enabled: true
    ToolTip.delay: 1000
    ToolTip.timeout: 5000
    ToolTip.visible: ToolTip.text && (down || hovered)
    ToolTip.text: toolTip


    // actions

    onClicked: triggered()


    onCheckedChanged: if(checked) activated()

    onActiveFocusChanged: focusTimer.start()

    Timer {
        id: focusTimer
        interval: 100
        onTriggered: {
            if(control.pressed) start()
            else control.focus=false
        }
    }

    // contents

    property Component contentComponent
    Component.onCompleted: {
        if(contentComponent)
            contentItem = contentComponent.createObject(control)
    }

}
