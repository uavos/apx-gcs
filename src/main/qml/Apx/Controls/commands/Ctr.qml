import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0
import Apx.Menu 1.0

ListView {
    id: listView
    orientation: ListView.Horizontal
    model: model.model
    spacing: 4

    implicitHeight: 32
    implicitWidth: 100

    delegate: CleanButton {
        defaultHeight: listView.height
        ui_scale: 1
        text: modelData.title
        toolTip: modelData.descr?modelData.descr:modelData.title
        iconName: modelData.icon
        highlighted: modelData.active
        showText: false
        color: Qt.darker(Material.color(Material.Blue),2.2)
        onTriggered: {
            Menu.show(modelData,{},root)
        }
    }

    MenuModel {
        id: model
    }
}
