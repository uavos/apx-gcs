import QtQuick 2.11
import QtQuick.Layouts 1.3

ColumnLayout {
    id: flow
    spacing: 8
    Repeater {
        property var list: flow.controls[flow.key]
        model: list?list:flow.defaultControls
        Loader {
            Layout.alignment: (Qt.AlignRight|Qt.AlignTop)
            sourceComponent: modelData
        }
    }
    property string key: ""
    property var defaultControls: []
    property var controls: ({})
}
