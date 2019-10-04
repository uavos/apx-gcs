import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Qt.labs.settings 1.0
import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import Apx.Common 1.0

Flow {
    id: control
    spacing: 8
    clip: true

    property alias settingsName: numbersModel.settingsName
    property alias defaults: numbersModel.defaults

    Repeater {
        model: numbersModel
    }

    CleanButton {
        implicitHeight: numbersModel.itemHeight
        implicitWidth: implicitHeight
        iconName: "note-plus-outline"//"plus-circle"
        toolTip: qsTr("Edit display values")
        onTriggered: numbersModel.edit()
        opacity: ui.effects?(hovered?1:0.5):1
    }


    NumbersModel {
        id: numbersModel
        itemHeight: 28*ui.scale
        light: true
    }
}
