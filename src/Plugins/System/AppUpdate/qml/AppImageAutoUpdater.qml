import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import AppImageAutoUpdater 1.0

import Apx.Common 1.0

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
            visible: text && root.state === AppImageAutoUpdater.UpdateAvailable
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
                url="https://uavos.github.io/apx-releases/notes/release-"+ver+".html"
            }
        }*/

        /*Item {
            Layout.fillHeight: true
        }*/

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
                fact: apx.sysmenu.app.install
                toolTip: application.installDir()
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
