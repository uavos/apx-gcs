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
import AppImageAutoUpdater

import Apx.Common

Pane {
    id: root
    anchors.fill: parent
    implicitWidth: 300
    implicitHeight: 400
    property var updater: apx.settings.application.appupdate.appimage_updater
    property var state: updater.state

    readonly property bool installed: application.installed()

    onStateChanged: {
        if(state === AppImageAutoUpdater.CheckForUpdates)
        {
            label.text = qsTr("Scan for updates...")
        }
        else if(state === AppImageAutoUpdater.UpdateAvailable)
        {
            label.text = qsTr("New version available.<br>Start update now?")
        }
        else if(state === AppImageAutoUpdater.NoUpdates)
        {
            label.text = qsTr("No updates available.")
        }
        else if(state === AppImageAutoUpdater.Updating)
        {
            label.text = qsTr("Self-update running, please wait...")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            id: label
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.fillHeight: false
            Layout.fillWidth: true
            ProgressBar {
                visible: root.state === AppImageAutoUpdater.CheckForUpdates || root.state === AppImageAutoUpdater.Updating
                indeterminate: root.state === AppImageAutoUpdater.CheckForUpdates
                value: updater.updateProgress
                from: 0
                to: 100
                Layout.fillWidth: true
            }
            Label {
                visible: root.state === AppImageAutoUpdater.Updating
                text: updater.updateProgress + "%"
            }
        }

        Label {
            Layout.fillHeight: false
            Layout.maximumWidth: 300
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            text: updater.statusMessage
            visible: root.state === AppImageAutoUpdater.Updating
        }

        Label {
            Layout.fillHeight: false
            Layout.fillWidth: true
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            property string sver: updater.latestVersion
            text: qsTr("Latest version")+": "+sver
            visible: sver && root.state === AppImageAutoUpdater.UpdateAvailable
        }

        ScrollView {
            id: flick
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            visible: edit.text && root.state === AppImageAutoUpdater.UpdateAvailable
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            TextArea {
                id: edit
                width: flick.width
                wrapMode: Text.WordWrap
                readOnly: true
                text: updater.releaseNotes
            }
        }
        /*WebView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            property string ver: updater.latestVersion
            onVerChanged: {
                if(!ver)return
                url="https://uavos.github.io/apx-gcs/docs/releases/release-"+ver+".html"
            }
        }*/

        Item {
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            visible: !installed
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Application is not installed")
            }
            FactButton {
                Layout.alignment: Qt.AlignHCenter
                fact: apx.sysmenu.app.install?apx.sysmenu.app.install:null
                toolTip: application.installDir()
                visible: fact
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            visible: installed
            Button {
                text: qsTr("Don't ask me again")
                visible: root.state === AppImageAutoUpdater.UpdateAvailable
                onClicked: {
                    apx.settings.application.appupdate.auto.value = false;
                    updater.menuBack();
                }
            }
            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Update!")
                visible: root.state === AppImageAutoUpdater.UpdateAvailable
                onClicked: updater.start()
            }
            Button {
                text: qsTr("Cancel")
                visible: root.state === AppImageAutoUpdater.Updating
                onClicked: updater.stop()
            }
        }
    }
}
