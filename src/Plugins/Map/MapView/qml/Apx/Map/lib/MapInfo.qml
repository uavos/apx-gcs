import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

RowLayout {
    Text {
        Layout.preferredWidth: height*8
        font.family: font_narrow
        //font.pixelSize: 13
        color: "#fff"
        property var c: map.mouseCoordinate
        text: apx.latToString(c.latitude)+" "+apx.lonToString(c.longitude)
    }

    MapScale { }

    Text {
        Layout.preferredWidth: height*3
        font.family: font_condenced
        color: "#fff"
        property var c: map.mouseCoordinate
        property var c0: map.mouseClickCoordinate
        text: apx.distanceToString(c0.distanceTo(c))
        horizontalAlignment: Qt.AlignHCenter
        ToolTipArea {
            text: qsTr("Distance between points")
        }
    }
}
