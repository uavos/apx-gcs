import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13
import AppImageAutoUpdater 1.0

Pane {
    id: root
    anchors.fill: parent
    implicitWidth: 300
    implicitHeight: 200
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

    Label {
        id: label
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Qt.AlignHCenter
    }
    RowLayout {
        anchors {
            top: label.bottom
            left: parent.left
            right: parent.right
        }
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

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        Button {
            text: qsTr("Don't ask me again")
            visible: root.state === AppImageAutoUpdater.UpdateAvailable
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
