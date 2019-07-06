import QtQuick 2.11
import QtQuick.Layouts 1.3

Flow {
    id: flow
    spacing: 8
    Repeater {
        property var list: flow.controls[flow.key]
        model: list?list:flow.defaultControls
        Loader {
            sourceComponent: modelData
        }
    }
    property string key: ""
    property var defaultControls: []
    property var controls: ({})
}
