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
        // columns: 8
        columns: 12
        anchors.fill: parent
        anchors.margins: Style.spacing * 1.2
        columnSpacing: Style.spacing * 1.2
        rowSpacing: Style.spacing * 1.2

        Repeater {
            // model: [
            //     "#ca2626", "#d69436", "#d6c544", "#74bf2f", "#6086af", "#926b8e", "#c59c5d", "#ffffff",
            //     "#ad0000", "#d06600", "#c9b300", "#61B218", "#2f568b", "#634568", "#a36914", "#b2b6af", 
            //     "#8b0000", "#ae4e00", "#a68700", "#43820c", "#1f4072", "#4e3057", "#794c08", "#9da09a",
            // ]

            model: [ // m.b. change: "#eee8aa", "#f0e68c", "#fffacd", "#ffffe0"(in yellow, orange), "#00fa9a"(in olive), "#dcdcdc"(in brown), "#f5f5f5"(in black)  
                "#ff7f50", "#db7093", "#f0e68c", "#fffacd", "#98fb98", "#00fa9a", "#add8e6", "#4682b4", "#9932cc", "#d8bfd8", "#dcdcdc", "#ffffff",
                "#ff6347", "#ff69b4", "#eee8aa", "#ffffe0", "#00ff00", "#6b8e23", "#87ceeb", "#0000ff", "#8a2be2", "#dda0dd", "#deb887", "#f5f5f5",
                "#ff4500", "#ff1493", "#ffa500", "#ffff00", "#32cd32", "#556b2f", "#00bfff", "#0000cd", "#9400d3", "#ee82ee", "#d2691e", "#808080",
                "#ff0000", "#dc143c", "#ff8c00", "#ffd700", "#008000", '#122c1d', "#1e90ff", "#000080", "#800080", "#ba55d3", "#a52a2a", "#000000"
            ]

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
