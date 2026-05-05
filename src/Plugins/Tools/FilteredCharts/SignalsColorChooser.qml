import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Apx.Common
import APX.Facts

Item {
    id: colorChooser

    property var color: fact.value != "Auto" ? fact.value : "transparent"
    property var space: Style.spacing * 1.2
    property var colorModel: createColorModel()
    readonly property var baseColors: [
        Material.Red,
        Material.Pink,
        Material.Purple,
        Material.DeepPurple,
        Material.Indigo,
        Material.Blue,
        Material.LightBlue,
        Material.Cyan,
        Material.Teal,
        Material.Green,
        Material.Orange,
        Material.BlueGrey
    ]
    readonly property var colorShades: [
        Material.Shade300,
        Material.Shade500,
        Material.Shade700,
        Material.Shade900
    ]

    implicitWidth: 480 * ui.scale
    implicitHeight: 132 * ui.scale

    function createColorModel() {
        var colors = []
        for (var i = 0; i < colorShades.length; ++i) {
            for (var j = 0; j < baseColors.length; ++j) {
                var colorCode = Material.color(baseColors[j], colorShades[i]).toString().toUpperCase()
                colors.push(colorCode)
            }
        }
        return colors;
    }

    function setAutoColor() {
        console.log("Number: add", fact.parentFact.num)
        console.log("Number add chart: ", fact.num)
        console.log("newItem", fact.parentFact.newItem)
        if (!fact || !fact.parentFact)
            return;
        var index = 0;
        var colorValue = "#FFFFFFF" 
        if (!fact.parentFact.newItem) {
            if(colorModel.length > 0) {
                index = fact.parentFact.num % colorModel.length
                colorValue = colorModel[index]
            } 
        } else {
            index = fact.parentFact.chartsCount
            colorValue = colorModel[index]
        }
        fact.value = colorValue;
        console.log("color:", fact.value)
    }
    
    Rectangle {
        id: colorBox
        border.width: 0
        color: "#282828"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: autoColor.top
        anchors.bottomMargin: space
        
        GridLayout {
            columns: 12
            anchors.fill: parent
            anchors.margins: space
            columnSpacing: space
            rowSpacing: space

            Repeater {
                model: colorModel

                delegate: Rectangle {
                    property var chosen: mouseArea.containsMouse 
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: modelData
                    radius: 2 * ui.scale
                    border.width: 1 * ui.scale
                    border.color: chosen ? "#b0c4de" : "transparent"
                    opacity: chosen ? 1 : 0.85
                    scale: chosen ? 1.15 : 1.0

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: { 
                            fact.value = modelData;
                            fact.menuBack()
                        }       
                    }
                }
            }
        }
    }
    Row {
        id: autoColor
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        spacing: space
        
        Rectangle {
            color: "transparent"
            width:  39 * ui.scale
            height: 23 * ui.scale
            radius: height / 5
            border.width: ui.scale
            border.color: Material.hintTextColor
            ToolTip.visible: maAutoColor.containsMouse
            ToolTip.text: qsTr("Use automatic series color")
            opacity: 0.85
            
            Text {
                anchors.centerIn: parent
                text: qsTr("A")
                font: apx.font_narrow(Math.max(10, parent.height * 0.55))
                color: Material.hintTextColor
            }
            MouseArea {
                id: maAutoColor
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    setAutoColor()
                    fact.menuBack()
                }       
            }
        }
        
        Label {
            text: "Auto color selection"
            font.pixelSize: Style.fontSize * 0.7
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // RowLayout {
    //     id: colorPreview
    //     anchors.bottom: parent.bottom
    //     spacing: space
        
    //     Rectangle {
    //         color: "transparent"
    //         width:  40 * ui.scale
    //         height: 24 * ui.scale
    //         radius: height / 5
    //         border.width: ui.scale
    //         // border.color: "#383838"
    //         border.color: Material.hintTextColor
    //         Layout.topMargin: space
    //         Layout.leftMargin: space
    //         opacity: 0.85
            
    //         Text {
    //             anchors.centerIn: parent
    //             text: qsTr("A")
    //             font: apx.font_narrow(Math.max(10, parent.height * 0.55))
    //             color: Material.hintTextColor
    //         }
    //     }
        
    //     Label {
    //         text: fact.value != "Auto" ? colorChooser.color : "Auto"
    //         font.pixelSize: Style.fontSize * 0.7
    //         Layout.topMargin: space
    //         Layout.alignment: Qt.AlignVCenter
    //     }
    // }
}
