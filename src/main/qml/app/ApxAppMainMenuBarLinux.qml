import QtQml 2.12
import QtQuick 2.11

import QtQuick.Controls 2.4

MenuBar {
    Menu {
        id: fileMenu
        title: qsTr("File")
        MenuItem {
            property var fact: apx.vehicles.REPLAY.telemetry.share.imp
            text: fact.descr
            action: Action { shortcut: StandardKey.Open }
            onTriggered: fact.trigger()
        }
        MenuItem {
            property var fact: apx.vehicles.REPLAY.nodes.share.imp
            text: fact.descr
            onTriggered: fact.trigger()
        }
        MenuItem {
            text: qsTr("Preferences")
            onTriggered: apx.requestMenu({"closeOnActionTrigger": false})
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
                onTriggered: modelData.requestMenu({"closeOnActionTrigger": false})
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
                var c = Qt.createComponent("AboutDialog.qml")
                if (c.status === Component.Ready){
                    c=c.createObject(control)
                    c.closed.connect(c.destroy)
                    c.open()
                }
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
