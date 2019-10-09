import QtQml 2.12
import QtQuick 2.11

import QtQuick.Controls 2.4

import Apx.Application 1.0

MenuBar {

    Component.onCompleted: manuBar=this

    Menu {
        id: fileMenu
        title: qsTr("File")
        MenuItem {
            property var fact: apx.vehicles.replay.telemetry.share.imp
            text: fact.descr
            action: Action { shortcut: StandardKey.Open }
            onTriggered: fact.trigger()
        }
        MenuItem {
            property var fact: apx.vehicles.replay.nodes.share.imp
            text: fact.descr
            onTriggered: fact.trigger()
        }
        MenuItem {
            text: qsTr("Preferences")
            onTriggered: apx.trigger()
        }
        Instantiator {
            active: apx.settings.application.updater !== undefined
            MenuItem {
                property var fact: apx.settings.application.updater
                text: fact ? fact.title : ""
                onTriggered: fact.check()
            }
            onObjectAdded: fileMenu.addItem(object)
        }
    }

    Menu {
        id: vehiclesMenu
        title: qsTr("Vehicle")
        Instantiator {
            model: apx.vehicles.select.model
            MenuItem {
                text: modelData.title
                onTriggered: modelData.trigger()
                checked: modelData.active
                checkable: true
            }
            onObjectAdded: vehiclesMenu.insertItem(index,object)
            onObjectRemoved: vehiclesMenu.removeItem(object)
        }
    }

    Menu {
        id: toolsMenu
        title: apx.tools.title
        Instantiator {
            model: apx.tools.model
            MenuItem {
                text: modelData.title
                onTriggered: modelData.trigger()
            }
            onObjectAdded: toolsMenu.insertItem(index,object)
            onObjectRemoved: toolsMenu.removeItem(object)
        }
    }

    Menu {
        id: windowsMenu
        title: apx.windows.title
        property var model: apx.windows.model
        Instantiator {
            model: windowsMenu.model
            MenuItem {
                text: modelData.title
                checked: modelData.value
                checkable: true
                onTriggered: modelData.value=!modelData.value
            }
            onObjectAdded: windowsMenu.insertItem(index,object)
            onObjectRemoved: windowsMenu.removeItem(object)
        }
    }

    Menu {
        title: qsTr("Help")
        MenuItem {
            text: qsTr("About")
            onTriggered: {
                var c=about.createObject(application.window)
                c.closed.connect(c.destroy)
                c.open()
            }
            property var about: Component {
                AboutDialog { }
            }
        }
        MenuItem {
            text: qsTr("Mandala Report")
            onTriggered: Qt.openUrlExternally("http://127.0.0.1:9080/mandala?descr")
        }
        MenuItem {
            text: qsTr("Documentation")
            onTriggered: Qt.openUrlExternally("http://docs.uavos.com")
            action: Action {shortcut: StandardKey.HelpContents}
        }
        MenuItem {
            text: qsTr("Report a problem")
            onTriggered: Qt.openUrlExternally("https://github.com/uavos/apx-releases/issues")
        }
    }
}
