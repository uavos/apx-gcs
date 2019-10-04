import QtQuick          2.12
import QtQuick.Layouts  1.12

RowLayout {
    Text {
        font: font_narrow
        color: "#fff"
        property var c: map.mouseCoordinate
        text: apx.latToString(c.latitude)+" "+apx.lonToString(c.longitude)
    }
}
