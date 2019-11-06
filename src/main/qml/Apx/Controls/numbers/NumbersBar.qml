import QtQuick 2.5;
import QtQuick.Layouts 1.3

import Qt.labs.settings 1.0
import QtQml.Models 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4

import Apx.Common 1.0

Flow {
    id: control
    spacing: itemSize*0.2
    clip: true

    property bool showEditButton: true
    property alias settingsName: numbersModel.settingsName
    property alias defaults: numbersModel.defaults
    property alias model: numbersModel

    property int itemSize: 28*ui.scale


    Loader {
        active: showEditButton
        visible: active
        sourceComponent: CleanButton {
            implicitHeight: numbersModel.itemHeight
            implicitWidth: implicitHeight
            iconName: "note-plus-outline"//"plus-circle"
            toolTip: qsTr("Edit display values")
            onTriggered: numbersModel.edit()
            opacity: ui.effects?(hovered?1:0.5):1
        }
    }

    Repeater {
        model: numbersModel
    }


    NumbersModel {
        id: numbersModel
        itemHeight: itemSize
        light: true
    }
}
