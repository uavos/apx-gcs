import QtQuick

Panel {
    required property var page

    title: qsTr("CURRENT PARAMETERS")

    ValueRow { label: qsTr("Azimuth"); value: page.currentYaw.toFixed(1) + "°" }
    ValueRow { label: qsTr("Elevation"); value: page.currentPitch.toFixed(1) + "°" }
    ValueRow { label: qsTr("Signal"); value: page.currentRssi.toFixed(0) + "%"; valueColor: "#ffd014" }
    ValueRow {
        label: qsTr("LOS range")
        value: page.losDistance > 0
               ? (page.losDistance >= 1000
                  ? (page.losDistance / 1000).toFixed(1) + " km"
                  : page.losDistance.toFixed(0) + " m")
               : "--"
        valueColor: "#a0b8e0"
    }
}
