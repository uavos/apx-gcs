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
    id: control

    implicitHeight: layout.implicitHeight
    implicitWidth: layout.implicitWidth

    border.width: 0
    color: "#000"

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        SignalsView {
            id: signals
            facts: []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 20
            Layout.preferredHeight: 130*ui.scale
        }

        TextInput {
            id: textInput

            Layout.fillWidth: true
            Layout.minimumHeight: Style.fontSize

            // clip: true
            // focus: true
            visible: false

            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            font: apx.font_narrow(Style.fontSize)

            color: activeFocus?Material.color(Material.Yellow):Material.primaryTextColor
            text: "est.air.airspeed"

            activeFocusOnTab: true
            selectByMouse: true

            onEditingFinished: {
                updateFacts()
            }
            onActiveFocusChanged: {
                if(activeFocus)selectAll();
            }
            onVisibleChanged: if(visible)forceActiveFocus()
            Component.onCompleted: updateFacts()

            property var facts: []
            function updateFacts()
            {
                var flist=[]
                var list=textInput.text.split(',')
                for(var i=0;i<list.length;++i){
                    var f=list[i]
                    var fact={}
                    if(eval(f)==undefined) continue
                    fact.title=f
                    fact.name=f
                    fact.descr=f
                    fact.opts={}
                    fact.opts.color=Material.color(Material.Blue+i*2)
                    flist.push(fact)
                }
                if(JSON.stringify(textInput.facts)==JSON.stringify(flist))
                    return
                textInput.facts=flist

                if(plusButton.checked)
                    signals.facts=flist
            }
        }

        ButtonGroup {
            id: buttonGroup
            //buttons: bottomArea.buttons
        }

        RowLayout {
            id: bottomArea
            Layout.fillWidth: true
            Layout.margins: Style.spacing
            spacing: 3
            Layout.maximumHeight: 24*ui.scale
            SignalButton {
                text: "R"
                values: [ mandala.fact("cmd.att.roll"), mandala.fact("est.att.roll") ]
            }
            SignalButton {
                text: "P"
                values: [ mandala.fact("cmd.att.pitch"), mandala.fact("est.att.pitch") ]
            }
            SignalButton {
                text: "Y"
                values: [ mandala.fact("cmd.pos.bearing"), mandala.fact("cmd.att.yaw"), mandala.fact("est.att.yaw") ]
            }
            SignalButton {
                text: "Axy"
                values: [ mandala.fact("est.acc.x"), mandala.fact("est.acc.y") ]
            }
            SignalButton {
                text: "Az"
                values: [ mandala.fact("est.acc.z") ]
            }
            SignalButton {
                text: "G"
                values: [ mandala.fact("est.gyro.x"), mandala.fact("est.gyro.y"), mandala.fact("est.gyro.z") ]
            }
            SignalButton {
                text: "Pt"
                values: [ mandala.fact("est.pos.altitude"), mandala.fact("est.pos.vspeed"), mandala.fact("est.air.airspeed") ]
            }
            SignalButton {
                text: "Ctr"
                values: [ mandala.fact("ctr.att.ail"), mandala.fact("ctr.att.elv"), mandala.fact("ctr.att.rud"), mandala.fact("ctr.eng.thr"), mandala.fact("ctr.eng.prop"), mandala.fact("ctr.str.rud") ]
            }
            SignalButton {
                text: "RC"
                values: [ mandala.fact("cmd.rc.roll"), mandala.fact("cmd.rc.pitch"), mandala.fact("cmd.rc.thr"), mandala.fact("cmd.rc.yaw") ]
            }
            SignalButton {
                text: "Usr"
                values: [ mandala.fact("est.usr.u1"), mandala.fact("est.usr.u2"), mandala.fact("est.usr.u3"), mandala.fact("est.usr.u4"), mandala.fact("est.usr.u5"), mandala.fact("est.usr.u6") ]
            }

            SignalButton {
                id: plusButton
                text: "+"
                values: textInput.facts
                onCheckedChanged: {
                    if(!checked)
                        textInput.visible=false
                }
                onPressed: {
                    if(checked)
                        textInput.visible=!textInput.visible
                }
            }

            TextButton {
                text: signals.speedFactorValue+"x"
                onClicked: signals.changeSpeed()
                Layout.fillHeight: true
                Layout.minimumWidth: height*3
            }
        }
    }

    property string currentPage: buttonGroup.checkedButton.text

    Settings {
        category: "signals"
        property alias page: control.currentPage
        property alias custom: textInput.text
    }
    Component.onCompleted: {
        for(var i=0;i<buttonGroup.buttons.length;++i){
            var b=buttonGroup.buttons[i]
            if(b.text!==control.currentPage)continue
            buttonGroup.checkedButton=b
            break
        }
        if(buttonGroup.checkedButton==null){
            buttonGroup.checkedButton=buttonGroup.buttons[0] //showPage("R")
        }
    }

}
