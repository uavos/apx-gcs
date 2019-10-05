import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

RowLayout {

    readonly property int iconSize: text.implicitHeight*0.8


    MaterialIcon {
        name: "chevron-left"
        size: iconSize
    }

    Text {
        id: text
        Layout.fillHeight: true
        Layout.fillWidth: true
        font.family: font_condenced
        color: "#fff"
        property var c: map.mouseCoordinate
        property var c0: map.mouseClickCoordinate
        text: apx.distanceToString(c0.distanceTo(c))
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        ToolTipArea {
            text: qsTr("Distance between points")
        }
    }

    MaterialIcon {
        name: "chevron-right"
        size: iconSize
    }
}
