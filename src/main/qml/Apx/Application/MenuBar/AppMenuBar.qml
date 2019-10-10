import QtQuick 2.12
import QtQml 2.12

import APX.Facts 1.0

Loader {
    asynchronous: false
    source: {
        var cname="MenuBar"
        if(Qt.platform.os=="linux")cname+="Linux"
        else cname+="Macos"
        return cname+".qml"
    }


    /*Fact {
        name: "menubar"
        flags: Fact.Group

        Fact {
            title: qsTr("File")
            Fact {
                property var fact: apx.vehicles.replay.telemetry.share.imp
                title: fact.descr
                opts: {
                    "shortcut": StandardKey.Open
                }
                onTriggered: fact.trigger()
            }
            Fact {
                property var fact: apx.vehicles.replay.nodes.share.imp
                title: fact.descr
                onTriggered: fact.trigger()
            }
            Fact {
                title: qsTr("Preferences")
                section: "Preferences"
                onTriggered: apx.trigger()
            }
            Fact {
                property var fact: apx.settings.application.updater
                visible: fact?true:false
                section: "ApplicationSpecific"
                title: fact?fact.title:""
                onTriggered: fact.check()
            }
        }

        Fact {
            id: vehicles
            title: qsTr("Vehicle")
            model: apx.vehicles.select.model
        }

        Fact {
            id: toolsMenu
            title: apx.tools.title
            Instantiator {
                model: apx.tools.model
                Fact {
                    title: modelData.title
                    onTriggered: modelData.trigger()
                }
                onObjectAdded: toolsMenu.insertItem(index,object)
                onObjectRemoved: toolsMenu.removeItem(object)
            }
        }

        Fact {
            id: controlsMenu
            title: apx.windows.title
            Instantiator {
                model: apx.windows.model
                Fact {
                    title: modelData.title
                    checked: modelData.value
                    checkable: true
                    onTriggered: modelData.value=!modelData.value
                }
                onObjectAdded: controlsMenu.insertItem(index,object)
                onObjectRemoved: controlsMenu.removeItem(object)
            }
        }

        Fact {
            title: qsTr("Help")
            Fact {
                role: MenuItem.AboutRole
                onTriggered: {
                    var c=about.createObject(application.window)
                    c.closed.connect(c.destroy)
                    c.open()
                }
                property var about: Component {
                    AboutDialog { }
                }
            }
            Fact {
                title: qsTr("Mandala Report")
                onTriggered: Qt.openUrlExternally("http://127.0.0.1:9080/mandala?descr")
            }
            Fact {
                title: qsTr("Documentation")
                onTriggered: Qt.openUrlExternally("http://docs.uavos.com")
                opts: { "shortcut": StandardKey.HelpContents }
            }
            Fact {
                title: qsTr("Changelog")
                onTriggered: Qt.openUrlExternally("http://uavos.github.io/apx-releases/CHANGELOG.html")
            }
            Fact {
                title: qsTr("Report a problem")
                onTriggered: Qt.openUrlExternally("https://github.com/uavos/apx-releases/issues")
            }
        }
    }*/
}
