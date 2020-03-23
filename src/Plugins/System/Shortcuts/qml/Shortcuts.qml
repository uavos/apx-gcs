import QtQuick 2.12
import QtQml 2.12

import Apx.Common 1.0

Loader {

    property var plugin: apx.settings.interface.shortcuts
    active: plugin && !plugin.blocked.value

    sourceComponent: sys

    Loader {
        sourceComponent: usr
    }

    Component {
        id: sys
        Instantiator {
            model: plugin?plugin.system.model:null
            Shortcut {
                context: Qt.ApplicationShortcut
                enabled: modelData.enb.value
                sequence: modelData.key.text
                onActivated: application.jsexec(modelData.jscmd.text)
            }
        }
    }

    Component {
        id: usr
        Instantiator {
            model: plugin?plugin.user.model:null
            Shortcut {
                context: Qt.ApplicationShortcut
                enabled: modelData.enb.value
                sequence: modelData.key.text
                onActivated: application.jsexec(modelData.jscmd.text)
            }
        }
    }
}
