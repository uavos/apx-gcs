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
import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0
import Apx.Menu 1.0

import APX.Vehicles 1.0 as APX


Rectangle {
    id: root

    readonly property var f_mode: mandala.cmd.proc.mode


    border.width: 0
    color: "#000"
    implicitWidth: 400
    implicitHeight: 400

    onWidthChanged: implicitHeight=width
    onHeightChanged: implicitWidth=height

    readonly property real margins: width/200

    property APX.Vehicle vehicle: apx.vehicles.current

    //sizes
    readonly property real buttonHeight: width/10
    readonly property real buttonSpacing: buttonHeight/10

    property real size: Math.max(50,width)*0.08

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.margins
        spacing: buttonSpacing*2


        RowLayout {
            Layout.alignment: Qt.AlignTop|Qt.AlignLeft
            height: buttonHeight
            spacing: buttonSpacing*2

            ValueButton {
                size: buttonHeight
                fact: vehicle
                toolTip: apx.vehicles.title
                showText: false
                value: vehicle.title
                active: false
                warning: vehicle.streamType<=0
                enabled: true
            }

            ValueButton {
                size: buttonHeight
                fact: vehicle.nodes
                showText: false
                showIcon: true
                //valueScale: 0.6
                value: fact.value?fact.value:""
                warning: vehicle.isIdentified && !value
                active: fact.modified || fact.progress>=0 || (!fact.valid)
                enabled: true
            }

            ValueButton {
                size: buttonHeight
                fact: vehicle.mission
                showText: false
                showIcon: true
                //valueScale: 0.6
                value: fact.missionSize
                warning: fact.missionSize<=0
                active: fact.modified || fact.progress>=0
                enabled: true
            }
        }

        //Mode change row
        StackView {
            clip: true
            Layout.fillWidth: true
            implicitHeight: buttonHeight
            property string mode: f_mode.text
            onModeChanged: {
                var modes
                var body
                switch(f_mode.value){
                default: modes=f_mode.enumStrings; break;
                case proc_mode_EMG:
                    modes=["RPV","TAXI"]
                    body="EMG"
                    break
                case proc_mode_RPV:
                    modes=["UAV","WPT"]
                    break
                case proc_mode_UAV:
                    modes=["WPT","LANDING"]
                    break
                case proc_mode_WPT:
                    modes=["STBY","LANDING"]
                    break
                case proc_mode_STBY:
                    modes=["WPT","LANDING"]
                    break
                case proc_mode_TAXI:
                    modes=["TAKEOFF","EMG"]
                    break
                case proc_mode_TAKEOFF:
                    modes=["WPT","STBY"]
                    break
                case proc_mode_LANDING:
                    modes=["WPT","STBY"]
                    break
                }
                replace(null,"Mode.qml",{"modes": modes, "body": body },StackView.PushTransition)
            }
        }

        //body controls area
        Pages {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        //menus
        MenuButtons {
            Layout.fillWidth: true
            Layout.preferredHeight: buttonHeight*1.2
        }

    }
}
