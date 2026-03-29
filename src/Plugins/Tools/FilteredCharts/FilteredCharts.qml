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
    id: fcControl

    implicitHeight: layout.implicitHeight
    implicitWidth: layout.implicitWidth

    border.width: 0
    color: "#000"

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        FcChartsView {
            id: fcCharts
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130 * ui.scale
        }

        TextInput {
            id: textInput

            Layout.fillWidth: true
            Layout.minimumHeight: Style.fontSize
            Layout.rightMargin: Style.spacing

            // clip: true
            // focus: true
            visible: false

            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            font: apx.font_narrow(Style.fontSize)

            color: activeFocus ? Material.color(Material.Yellow) : Material.primaryTextColor
            text: ""

            activeFocusOnTab: true
            selectByMouse: true

            onEditingFinished: {
                var activeButton = buttonGroup.checkedButton;
                if (activeButton)
                    activeButton.toolTip = textInput.text;
                // updateFacts();
            }
            onActiveFocusChanged: {
                if (activeFocus)
                    selectAll();
            }
            onVisibleChanged: if (visible)
                forceActiveFocus()

            Component.onCompleted: updateFacts()

            property var facts: []
            function updateFacts() {
                var flist = [];
                var list = textInput.text.split(',');
                for (var i = 0; i < list.length; ++i) {
                    var f = list[i];
                    var fact = {};
                    if (eval(f) == undefined)
                        continue;
                    fact.title = f;
                    fact.name = f;
                    fact.descr = f;
                    fact.opts = {};
                    fact.opts.color = Material.color(Material.Blue + i * 2);
                    flist.push(fact);
                }
                if (JSON.stringify(textInput.facts) == JSON.stringify(flist))
                    return;
                textInput.facts = flist;

                if (plusButton.checked)
                    fcCharts.facts = flist;
            }
        }

        ButtonGroup {
            id: buttonGroup
            // buttons: bottomArea.buttons
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24 * ui.scale
            FcButton {
                text: "1"
                checked: true
                values: [mandala.cmd.att.roll, mandala.est.att.roll]
            }
            FcButton {
                text: "2"
                values: [mandala.cmd.att.pitch, mandala.est.att.pitch]
            }
            FcButton {
                text: "3"
                values: [mandala.cmd.pos.bearing, mandala.cmd.att.yaw, mandala.est.att.yaw]
            }
            FcButton {
                text: "4"
                values: [mandala.est.acc.x, mandala.est.acc.y]
            }
            FcButton {
                text: "5"
                values: [mandala.est.acc.z]
            }
            FcButton {
                text: "6"
                values: [mandala.est.gyro.x, mandala.est.gyro.y, mandala.est.gyro.z]
            }
            FcButton {
                text: "7"
                values: [mandala.est.pos.altitude, mandala.est.pos.vspeed, mandala.est.air.airspeed]
            }
            FcButton {
                text: "8"
                values: [mandala.ctr.att.ail, mandala.ctr.att.elv, mandala.ctr.att.rud, mandala.ctr.eng.thr, mandala.ctr.eng.prop, mandala.ctr.str.rud]
            }
            FcButton {
                text: "9"
                values: [mandala.cmd.rc.roll, mandala.cmd.rc.pitch, mandala.cmd.rc.thr, mandala.cmd.rc.yaw]
            }
            FcButton {
                text: "10"
                values: [mandala.est.usr.u1, mandala.est.usr.u2, mandala.est.usr.u3, mandala.est.usr.u4, mandala.est.usr.u5, mandala.est.usr.u6]
            }

            TextButton {
                text: fcCharts.speedFactorValue + "x"
                // onClicked: fcCharts.changeSpeed()
                Layout.fillHeight: true
                Layout.minimumWidth: height * 3
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
        onTriggered: console.log("Not implemented")
    }

    property string currentPage: buttonGroup.checkedButton.text

    Settings {
        category: "filtredCharts"
        property alias page: fcControl.currentPage
        property alias custom: textInput.text
    }
    Component.onCompleted: {
        for (var i = 0; i < buttonGroup.buttons.length; ++i) {
            var b = buttonGroup.buttons[i];
            if (b.text !== fcControl.currentPage)
                continue;
            buttonGroup.checkedButton = b;
            break;
        }
        if (buttonGroup.checkedButton == null) {
            buttonGroup.checkedButton = buttonGroup.buttons[0]; //showPage("R")
        }
    }
}
