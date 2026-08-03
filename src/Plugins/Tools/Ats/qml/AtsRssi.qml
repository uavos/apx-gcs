import QtQuick

import "rssi"

Item {
    id: root

    property var fact

    property var _window: null

    Connections {
        target: fact
        function onValueChanged() {
            if (fact.value) {
                if (!root._window) {
                    root._window = windowComponent.createObject(null, { fact: root.fact })
                } else {
                    root._window.visible = true
                }
            } else {
                if (root._window)
                    root._window.visible = false
            }
        }
    }

    Component {
        id: windowComponent

        RssiWindow {}
    }
}
