import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import AppImageAutoUpdater 1.0

Pane {
    id: root
    anchors.fill: parent
    implicitWidth: 300
    implicitHeight: 400
    property var updater: apx.settings.application.appupdate.appimage_updater
    property var state: updater.state
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

        Switch {
            id: keepOldVersionSwitch
            Layout.fillHeight: false
            Layout.fillWidth: true
            text: qsTr("Keep current version")
            checked: false
            visible: root.state === AppImageAutoUpdater.UpdateAvailable
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
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
                onClicked: updater.start(keepOldVersionSwitch.checked)
            }
            Button {
                text: qsTr("Cancel")
                visible: root.state === AppImageAutoUpdater.Updating
                onClicked: updater.stop()
            }
        }
    }
}
