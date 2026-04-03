import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Apx.Common
import APX.Facts

Rectangle {
    id: colorBox
    border.width: 0
    color: "#282828"
    implicitWidth: 420
    implicitHeight: 95
    // implicitHeight: 120

    property var chartColor: "white"

    GridLayout {
        columns: 8
        // columns: 12
        anchors.fill: parent
        anchors.margins: Style.spacing * 1.2
        columnSpacing: Style.spacing * 1.2
        rowSpacing: Style.spacing * 1.2

        Repeater {
            model: [
                "#ca2626", "#d69436", "#d6c544", "#74bf2f", "#6086af", "#926b8e", "#c59c5d", "#ffffff",
                "#ad0000", "#d06600", "#c9b300", "#61B218", "#2f568b", "#634568", "#a36914", "#b2b6af", 
                "#8b0000", "#ae4e00", "#a68700", "#43820c", "#1f4072", "#4e3057", "#794c08", "#9da09a"
            ]

            // model: ["#ff0000", "#dc143c", "#ff6347", "#ff7f50", "#ff4500", "#ff1493", "#ff69b4", "#db7093", 
            //         "#ff8c00", "#ffa500", "#ffd700", "#ffff00", "#ffffe0", "#fffacd", "#eee8aa", "#f0e68c", 
            //         "#008000", "#00ff00", "#32cd32", "#98fb98", "#00fa9a", "#2e8b57", "#6b8e23", "#556b2f", 
            //         "#0000ff", "#0000cd", "#1e90ff", "#00bfff", "#87ceeb", "#add8e6", "#4682b4", "#000080", 
            //         "#800080", "#8a2be2", "#9400d3", "#9932cc", "#ba55d3", "#d8bfd8", "#dda0dd", "#ee82ee", 
            //         "#a52a2a", "#d2691e", "#deb887", "#ffffff", "#f5f5f5", "#dcdcdc", "#808080", "#000000"]

            delegate: Rectangle {
                property var chosen: mouseArea.containsMouse 
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: height/12
                color: modelData
                border.width: ui.scale >= 1 ? ui.scale : 1
                border.color: chosen ? "lightblue" : "transparent"
                opacity: chosen ? 1 : 0.8
                scale: chosen ? 1.1 : 1.0

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: fact.setValue(modelData);        
                }
            }
        }
    }
}
