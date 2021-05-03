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

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: list.implicitWidth+margins*2
    implicitHeight: 200

    property real margins: Style.spacing

    property real itemSize: Style.buttonSize*0.7
    property bool showEditButton: true

    property alias settingsName: numbersModel.settingsName
    property alias defaults: numbersModel.defaults
    property alias model: numbersModel

    NumbersModel {
        id: numbersModel
        itemHeight: control.itemSize
        fixedWidth: true
        defaults: [
            {"bind":"est.usr.u1","prec":"2"},
            {"bind":"est.usr.u2","prec":"2"},
            {"bind":"est.usr.u3","prec":"2"},
            {"bind":"est.usr.u4","prec":"2"},
            {"bind":"(Math.abs(cmd.rc.roll)+Math.abs(cmd.rc.pitch))/2","title":"RC","prec":"2","warn":"value>0.2","alarm":"value>0.5"},
        ]
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: control.margins
        implicitWidth: contentItem.childrenRect.width //numbersModel.minimumWidth
        clip: true
        spacing: 0
        model: numbersModel
        snapMode: ListView.SnapToItem
        ScrollBar.vertical: ScrollBar { width: 6 }
        footer: Loader{
            active: showEditButton
            sourceComponent: TextButton {
                text: "+"
                size: control.itemSize
                toolTip: qsTr("Edit display values")
                onTriggered: numbersModel.edit()
            }
        }
    }
}
