import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.2

import Apx.Common 1.0

Rectangle {
    id: control
    border.width: 0
    color: "#000"
    implicitWidth: itemWidth
    Layout.margins: 1

    readonly property real maxItems: 17.5
    readonly property real aspectRatio: 4

    readonly property real itemWidth: height/maxItems*aspectRatio
    readonly property real itemHeight: width/aspectRatio

    property alias settingsName: numbersModel.settingsName

    NumbersModel {
        id: numbersModel
        itemWidth: control.itemWidth
        itemHeight: itemWidth/aspectRatio
        defaults: [
            {"bind":"user1","prec":"2","title":"u1"},
            {"bind":"user2","prec":"2","title":"u2"},
            {"bind":"user3","prec":"2","title":"u3"},
            {"bind":"user4","prec":"2","title":"u4"},
            {"bind":"(Math.abs(m.rc_roll.value)+Math.abs(m.rc_pitch.value))/2","title":"RC","prec":"2","warn":"value>0.2","alarm":"value>0.5"},
        ]
    }

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        spacing: 0
        model: numbersModel
        snapMode: ListView.SnapToItem
        ScrollBar.vertical: ScrollBar { width: 6 }
        footer: NumbersItem {
            title: " +"
            height: itemHeight
            minimumWidth: itemWidth
            enabled: true
            toolTip: qsTr("Edit display values")
            onTriggered: numbersModel.edit()
        }

    }

}
