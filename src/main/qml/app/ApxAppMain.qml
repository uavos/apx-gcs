import QtQml 2.12
import QtQuick 2.11

import QtQuick.Controls 2.4

import Apx.Common 1.0
import Apx.Menu 1.0


Item {
    id: control

    //fact system menu support
    Connections {
        target: apx
        onFactTriggered: {
            Menu.show(fact,opts)
        }
    }


    //Shortcuts
    property bool shortcutsEnabled: !apx.settings.interface.shortcuts.blocked.value
    Instantiator {
        model: apx.settings.interface.shortcuts.system.model
        Shortcut {
            context: Qt.ApplicationShortcut
            enabled: modelData.enb.value && shortcutsEnabled
            sequence: modelData.key.text
            onActivated: application.jsexec(modelData.jscmd.text)
        }
    }
    Instantiator {
        model: apx.settings.interface.shortcuts.user.model
        Shortcut {
            context: Qt.ApplicationShortcut
            enabled: modelData.enb.value && shortcutsEnabled
            sequence: modelData.key.text
            onActivated: application.jsexec(modelData.jscmd.text)
        }
    }

    //Contents
    Loader {
        id: contents
        active: true
        asynchronous: true
        source: typeof(qmlMainFile)!='undefined'?qmlMainFile:"GroundControl.qml"
        anchors.fill: parent
        onLoaded: application.registerUiComponent(item,"main")
    }
}
