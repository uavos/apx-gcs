import QtQml 2.12
import QtQuick 2.11

import Qt.labs.platform 1.1

Item //external Item wrapper needed, because ApplicationWindow expect classic, non-native MenuBar
{
    MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                property var fact: apx.vehicles.REPLAY.telemetry.share.imp
                text: fact.descr
                shortcut: StandardKey.Open
                onTriggered: fact.trigger()
            }
            MenuItem {
                property var fact: apx.vehicles.REPLAY.nodes.share.imp
                text: fact.descr
                onTriggered: fact.trigger()
            }
            MenuItem {
                role: MenuItem.PreferencesRole
                onTriggered: apx.requestMenu({"closeOnActionTrigger": false})
            }
            MenuItem {
                property var fact: apx.settings.application.updater
                visible: fact?true:false
                role: MenuItem.ApplicationSpecificRole
                text: fact?fact.title:""
                onTriggered: fact.check()
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
            id: controlsMenu
            title: apx.windows.title
            Instantiator {
                model: apx.windows.model
                MenuItem {
                    text: modelData.title
                    checked: modelData.value
                    onTriggered: modelData.value=!modelData.value
                }
                onObjectAdded: controlsMenu.insertItem(index,object)
                onObjectRemoved: controlsMenu.removeItem(object)
            }
        }

        Menu {
            title: qsTr("Help")
            MenuItem {
                role: MenuItem.AboutRole
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
                shortcut: StandardKey.HelpContents
            }
            MenuItem {
                text: qsTr("Changelog")
                onTriggered: Qt.openUrlExternally("http://uavos.github.io/apx-releases/CHANGELOG.html")
                shortcut: StandardKey.HelpContents
            }
        }
    }
}
