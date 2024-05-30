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
import QtQuick.Layouts
import QtQuick.Controls.Material

Dialog {
    id: dialog

    title: qsTr("About")+" "+Qt.application.name
    standardButtons: Dialog.Close

    property var w: parent
    x: (w.width-width)/2
    y: (w.height-height)/2


    implicitWidth: Math.max(w.width/4,800)
    //implicitHeight: Math.max(parent.height/4,100)

    contentItem: RowLayout {
        Image {
            Layout.alignment: Qt.AlignTop|Qt.AlignLeft
            Layout.preferredWidth: 128
            Layout.preferredHeight: 128
            source: "qrc:/icons/uavos-logo.ico"
            fillMode: Image.PreserveAspectFit
        }
        TextEdit {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Material.foreground
            text: application.aboutString()
            font: dialog.font
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            selectByKeyboard: true
            readOnly: true
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}
